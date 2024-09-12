#pragma once
#include "my_cpp_utils/logger.h"
#include "net_message.h"
#include "net_thread_safe_queue.h"
#include <asio.hpp>
#include <asio/io_context.hpp>
#include <asio/write.hpp>
#include <cstdint>

namespace net
{

// Forward declare the server interface.
template <typename T>
class server_interface;

// Client and server depends on the connection class.
// Connection use net::thread_safe_queue and net::message.
template <typename T>
class connection : public std::enable_shared_from_this<connection<T>>
{
public:
    enum class owner
    {
        server,
        client
    };

    connection(
        owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket,
        thread_safe_queue<owned_message<T>>& qIn)
      : m_asioContext(asioContext), m_socket(std::move(socket)), m_qMessagesIn(qIn)
    {
        m_nOwnerType = parent;

        if (m_nOwnerType == owner::server)
        {
            // Server constuct random handshake number from the current time.
            m_nHandshakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());

            // Precalculate the result for the handshake validation.
            m_nHandshakeCheck = scramble(m_nHandshakeOut);
        }
        else
        {
            // Client does not need to calculate the handshake number.
            m_nHandshakeIn = 0;
            m_nHandshakeOut = 0;
        }
    }

    virtual ~connection() {}

    // Get the unique ID for this connection.
    uint32_t GetID() const { return m_nID; }
public:
    bool ConnectToClient(net::server_interface<T>* server, uint32_t uid = 0)
    {
        if (m_nOwnerType == owner::server)
        {
            if (m_socket.is_open())
            {
                m_nID = uid;

                // Send the handshake to the client.
                WriteValidation();

                // Read the handshake response from the client.
                ReadValidation(server);
            }
        }
        return false;
    }

    // Called by clients.
    bool ConnectToServer(asio::ip::tcp::resolver::results_type endpoints)
    {
        if (m_nOwnerType == owner::client)
        {
            MY_LOG(
                debug, "[Connection] ConnectToServer STARTS at {}:{}", endpoints->endpoint().address().to_string(),
                endpoints->endpoint().port());

            asio::async_connect(
                m_socket, endpoints,
                [this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
                {
                    if (!ec)
                    {
                        MY_LOG(
                            debug, "[Connection] ConnectToServer HAS COMPLETED at {}:{}",
                            endpoint.address().to_string(), endpoint.port());

                        ReadValidation();
                    }
                });
            return true;
        }
        return false;
    }

    // Called by both clients and servers.
    bool Disconnect()
    {
        if (IsConnected())
        {
            MY_LOG(debug, "[Connection] Disconnect STARTS from {}", m_socket.remote_endpoint().address().to_string());

            asio::post(m_asioContext, [this]() { m_socket.close(); });
            return true;
        }
        return false;
    }
    // Is the connection still active?
    bool IsConnected() const { return m_socket.is_open(); }

    // Send a message to the remote endpoint.
    void Send(const message<T>& msg)
    {
        MY_LOG(debug, "[Connection] Send STARTS: ID {}, BodySize {}", msg.header.id, msg.body.size());

        // Add new task to the ASIO context.
        asio::post(
            m_asioContext,
            [this, msg]()
            {
                // If the queue has a message in it, then we must
                // assume that it is in the process of asynchronously being written.
                bool bWritingMessage = !m_qMessagesOut.empty();
                m_qMessagesOut.push_back(msg);

                // log push_back message.
                MY_LOG(
                    debug, "[Connection] Send: ID {}, BodySize {}, QueueSize {}, WritingMessage {}", msg.header.id,
                    msg.body.size(), m_qMessagesOut.count(), bWritingMessage);

                MY_LOG(debug, "[Connection] Send HAS COMPLETED: ID {}, BodySize {}", msg.header.id, msg.body.size());

                if (!bWritingMessage)
                {
                    // Restart writing messages process if it's not already running.
                    // Suppose when the message queue is empty, the writing process is not running.
                    WriteHeader();
                }
            });
    }
private:
    // ASYNC - Prime context ready to read a message header.
    void ReadHeader()
    {
        MY_LOG(debug, "[Connection] ReadHeader STARTS");

        asio::async_read(
            m_socket, asio::buffer(&m_msgTemporaryIn.header, sizeof(message_header<T>)),
            [this](std::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    MY_LOG(
                        debug, "[Connection] ReadHeader HAS COMPLETED: ID {}, BodySize {}, AsioLenth {}",
                        m_msgTemporaryIn.header.id, m_msgTemporaryIn.header.size, length);

                    if (m_msgTemporaryIn.header.size > 0)
                    {
                        // Body in not empty, so resize the message buffer to hold the message body.
                        m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);
                        ReadBody();
                    }
                    else
                    {
                        // Body is empty, so add this message to the incoming message queue.
                        AddToIncomingMessageQueue();
                    }
                }
                else
                {
                    MY_LOG(error, "[Connection] ReadHeader HAS FAILED: {}", ec.message());
                    m_socket.close();
                }
            });
    }

    // ASYNC - Prime context ready to read a message body.
    void ReadBody()
    {
        MY_LOG(debug, "[Connection] ReadBody STARTS: BodySize {}", m_msgTemporaryIn.body.size());

        asio::async_read(
            m_socket, asio::buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()),
            [this](std::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    MY_LOG(
                        debug, "[Connection] ReadBody HAS COMPLETED: Size {}, AsioLenth {}",
                        m_msgTemporaryIn.body.size(), length);

                    // Body has been read, so add this message to the incoming message queue.
                    AddToIncomingMessageQueue();
                }
                else
                {
                    MY_LOG(error, "[Connection] ReadBody HAS FAILED: {}", ec.message());
                    m_socket.close();
                }
            });
    }

    // ASYNC - Prime context ready to write a message header.
    void WriteHeader()
    {
        MY_LOG(
            debug, "[Connection] WriteHeader STARTS: ID {}, BodySize {}", m_qMessagesOut.front().header.id,
            m_qMessagesOut.front().header.size);

        asio::async_write(
            m_socket, asio::buffer(&m_qMessagesOut.front().header, sizeof(message_header<T>)),
            [this](std::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    MY_LOG(
                        debug, "[Connection] WriteHeader HAS COMPLETED: ID {}, BodySize {}, AsioLenth {}",
                        m_qMessagesOut.front().header.id, m_qMessagesOut.front().header.size, length);

                    if (m_qMessagesOut.front().body.size() > 0)
                    {
                        WriteBody();
                    }
                    else
                    {
                        m_qMessagesOut.pop_front();

                        if (!m_qMessagesOut.empty())
                        {
                            WriteHeader();
                        }
                    }
                }
                else
                {
                    MY_LOG(error, "[Connection] WriteHeader HAS FAILED: {}", ec.message());
                    m_socket.close();
                }
            });
    }

    // ASYNC - Prime context ready to write a message body.
    void WriteBody()
    {
        MY_LOG(
            debug, "[Connection] WriteBody STARTS: ID {}, BodySize {}", m_qMessagesOut.front().header.id,
            m_qMessagesOut.front().body.size());

        asio::async_write(
            m_socket, asio::buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()),
            [this](std::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    MY_LOG(
                        debug, "[Connection] WriteBody HAS COMPLETED: ID {}, BodySize {}, AsioLenth {}",
                        m_qMessagesOut.front().header.id, m_qMessagesOut.front().body.size(), length);

                    m_qMessagesOut.pop_front();

                    if (!m_qMessagesOut.empty())
                    {
                        WriteHeader();
                    }
                }
                else
                {
                    MY_LOG(error, "[Connection] WriteBody HAS FAILED: {}", ec.message());
                    m_socket.close();
                }
            });
    }

    // Add a message to the incoming message queue.
    void AddToIncomingMessageQueue()
    {
        if (m_nOwnerType == owner::server)
        {
            MY_LOG(
                debug, "[Connection] Server received message: ID {}, BodySize {}, From client {}",
                m_msgTemporaryIn.header.id, m_msgTemporaryIn.body.size(), m_nID);
            m_qMessagesIn.push_back({this->shared_from_this(), m_msgTemporaryIn});
        }
        else
        {
            MY_LOG(
                debug, "[Connection] Client received message: ID {}, BodySize {}", m_msgTemporaryIn.header.id,
                m_msgTemporaryIn.body.size());
            // For client tagging the connection is not required.
            // Because the client has only one connection.
            m_qMessagesIn.push_back({nullptr, m_msgTemporaryIn});
        }

        // We must now prime the asio context to receive the next message.
        ReadHeader();
    }
private: // Encryption/Decryption.
    // Naive encrypt data function.
    uint64_t scramble(uint64_t nInput)
    {
        uint64_t out = nInput ^ 0xDEADBEEFC0DECAFE;
        out = (out & 0xF0F0F0F0DEADBEEF) << 8 | (out & 0xDEADBEEF00000000) >> 8;
        return out ^ 0xC0DEFACE12345678; // May use as version number for the protocol.
    }

    // ASYNC - Used by both client and server to write the handshake pattern.
    void WriteValidation()
    {
        MY_LOG(debug, "[Connection] WriteValidation STARTS: HandshakeOut {}", m_nHandshakeOut);

        asio::async_write(
            m_socket, asio::buffer(&m_nHandshakeOut, sizeof(uint64_t)),
            [this](std::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    MY_LOG(
                        debug, "[Connection] WriteValidation HAS COMPLETED: HandshakeOut {}, AsioLenth {}",
                        m_nHandshakeOut, length);

                    // Validation data sent. Client should sit and wait for a response.
                    if (m_nOwnerType == owner::client)
                        ReadHeader();
                }
                else
                {
                    MY_LOG(error, "[Connection] WriteValidation HAS FAILED: {}", ec.message());
                    m_socket.close();
                }
            });
    }

    void ReadValidation(net::server_interface<T>* server = nullptr)
    {
        MY_LOG(debug, "[Connection] ReadValidation STARTS");

        asio::async_read(
            m_socket, asio::buffer(&m_nHandshakeIn, sizeof(uint64_t)),
            [this, server](std::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    MY_LOG(
                        debug, "[Connection] ReadValidation HAS COMPLETED: HandshakeIn {}, AsioLenth {}",
                        m_nHandshakeIn, length);

                    if (m_nOwnerType == owner::server)
                    {
                        // For the server: m_nHandshakeIn received from the client.
                        if (m_nHandshakeIn == m_nHandshakeCheck)
                        {
                            MY_LOG(
                                info, "[Connection] ReadValidation: HandshakeIn {} == HandshakeCheck {}",
                                m_nHandshakeIn, m_nHandshakeCheck);
                            MY_LOG(info, "[Connection] ReadValidation: Handshake is validated");

                            // TODO0: Restore this line.
                            // server->OnClientValidated(this->shared_from_this());

                            // Handshake is validated, so start reading the header.
                            ReadHeader();
                        }
                        else
                        {
                            // TODO3: There may be code to adding the client to a blacklist.

                            MY_LOG(
                                error, "[Connection] ReadValidation: HandshakeIn {} != HandshakeCheck {}",
                                m_nHandshakeIn, m_nHandshakeCheck);
                            MY_LOG(error, "[Connection] ReadValidation: Handshake is not validated");
                            m_socket.close();
                        }
                    }
                    else
                    {
                        // For the client: m_nHandshakeIn received from the server.
                        m_nHandshakeOut = scramble(m_nHandshakeIn);

                        // Send the handshake back to the server for validation.
                        WriteValidation();
                    }
                }
                else
                {
                    MY_LOG(error, "[Connection] ReadValidation HAS FAILED: {}", ec.message());
                    m_socket.close();
                }
            });
    }
protected:
    // This context is shared with the whole asio instance.
    // Provided by the client or server interface.
    asio::io_context& m_asioContext;
    // Responsible for the ASIO stuff.
    asio::ip::tcp::socket m_socket;
    // This queue holds all messages to be sent to the remote side.
    thread_safe_queue<message<T>> m_qMessagesOut;
    // This queue holds all messages that have been received from the remote side.
    // Note it is a reference as the "owner" of this connection is expected to provide a queue.
    // Provided by the client or server interface.
    thread_safe_queue<owned_message<T>>& m_qMessagesIn;
    // The "temporary" incoming message (completed messages are transferred to incoming message queue).
    message<T> m_msgTemporaryIn;
    // The "owner" decides how some of the connection behaves.
    owner m_nOwnerType = owner::server;
    uint32_t m_nID = 0;
protected: //  Handshake validation.
    // What the connections whould be send output.
    uint64_t m_nHandshakeOut = 0;
    // What the connections has received as result.
    uint64_t m_nHandshakeIn = 0;
    // Use from the server side to validate the client.
    uint64_t m_nHandshakeCheck = 0;
};

} // namespace net