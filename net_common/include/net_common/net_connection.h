#pragma once
#include "net_message.h"
#include "net_thread_safe_queue.h"
#include <asio.hpp>
#include <asio/io_context.hpp>

namespace net
{

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
    }
    virtual ~connection() {}

    // Get the unique ID for this connection.
    uint32_t GetID() const { return m_nID; }
public:
    bool ConnectToClient(uint32_t uid = 0)
    {
        if (m_nOwnerType == owner::server)
        {
            if (m_socket.is_open())
            {
                m_nID = uid;
                ReadHeader();
            }
        }
        return false;
    }

    // Called by clients.
    bool ConnectToServer(asio::ip::tcp::resolver::results_type endpoints)
    {
        if (m_nOwnerType == owner::client)
        {
            asio::async_connect(
                m_socket, endpoints,
                [this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
                {
                    if (!ec)
                    {
                        ReadHeader();
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
            asio::post(m_asioContext, [this]() { m_socket.close(); });
            return true;
        }
        return false;
    }
    // Is the connection still active?
    bool IsConnected() const { return m_socket.is_open(); }
public:
    // Send a message to the remote endpoint.
    void Send(const message<T>& msg)
    {
        // Add new task to the ASIO context.
        asio::post(
            m_asioContext,
            [this, msg]()
            {
                // If the queue has a message in it, then we must
                // assume that it is in the process of asynchronously being written.
                bool bWritingMessage = !m_qMessagesOut.empty();
                m_qMessagesOut.push_back(msg);
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
        asio::async_read(
            m_socket, asio::buffer(&m_msgTemporaryIn.header, sizeof(message_header<T>)),
            [this](std::error_code ec, std::size_t length)
            {
                if (!ec)
                {
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
                    MY_LOG_FMT(error, "[Connection] Read Header Fail: {}", ec.message());
                    m_socket.close();
                }
            });
    }

    // ASYNC - Prime context ready to read a message body.
    void ReadBody()
    {
        asio::async_read(
            m_socket, asio::buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()),
            [this](std::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    // Body has been read, so add this message to the incoming message queue.
                    AddToIncomingMessageQueue();
                }
                else
                {
                    MY_LOG_FMT(error, "[Connection] Read Body Fail: {}", ec.message());
                    m_socket.close();
                }
            });
    }

    // ASYNC - Prime context ready to write a message header.
    void WriteHeader()
    {
        asio::async_write(
            m_socket, asio::buffer(&m_qMessagesOut.front().header, sizeof(message_header<T>)),
            [this](std::error_code ec, std::size_t length)
            {
                if (!ec)
                {
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
                    MY_LOG_FMT(error, "[Connection] Write Header Fail: {}", ec.message());
                    m_socket.close();
                }
            });
    }

    // ASYNC - Prime context ready to write a message body.
    void WriteBody()
    {
        asio::async_write(
            m_socket, asio::buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()),
            [this](std::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    m_qMessagesOut.pop_front();

                    if (!m_qMessagesOut.empty())
                    {
                        WriteHeader();
                    }
                }
                else
                {
                    MY_LOG_FMT(error, "[Connection] Write Body Fail: {}", ec.message());
                    m_socket.close();
                }
            });
    }

    // Add a message to the incoming message queue.
    void AddToIncomingMessageQueue()
    {
        if (m_nOwnerType == owner::server)
        {
            m_qMessagesIn.push_back({this->shared_from_this(), m_msgTemporaryIn});
        }
        else
        {
            // For client tagging the connection is not required.
            // Because the client has only one connection.
            m_qMessagesIn.push_back({nullptr, m_msgTemporaryIn});
        }

        // We must now prime the asio context to receive the next message.
        ReadHeader();
    }
protected:
    // Responsible for the ASIO stuff.
    asio::ip::tcp::socket m_socket;
    // This context is shared with the whole asio instance.
    // Provided by the client or server interface.
    asio::io_context& m_asioContext;
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
};

} // namespace net