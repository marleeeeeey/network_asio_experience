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
            }
        }
        return false;
    }

    // Called by clients.
    bool ConnectToServer();
    // Called by both clients and servers.
    bool Disconnect();
    // Is the connection still active?
    bool IsConnected() const { return m_socket.is_open(); }
public:
    // Send a message to the remote endpoint.
    void Send(const message<T>& msg);
protected:
    // Responsible for the ASIO stuff.
    asio::ip::tcp::socket m_socket;
    // This context is shared with the whole asio instance.
    // Provided by the client or server interface.
    asio::io_context& m_asioContext;
    // This queue holds all messages to be sent to the remote side.
    thread_safe_queue<T> m_qMessagesOut;
    // This queue holds all messages that have been received from the remote side.
    // Note it is a reference as the "owner" of this connection is expected to provide a queue.
    // Provided by the client or server interface.
    thread_safe_queue<owned_message<T>>& m_qMessagesIn;
    // The "owner" decides how some of the connection behaves.
    owner m_nOwnerType = owner::server;
    uint32_t m_nID = 0;
};

} // namespace net