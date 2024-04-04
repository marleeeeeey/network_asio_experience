#pragma once
#include "net_message.h"
#include "net_thread_safe_queue.h"
#include <asio.hpp>

namespace net
{

// Client and server depends on the connection class.
// Connection use net::thread_safe_queue and net::message.
template <typename T>
class connection : public std::enable_shared_from_this<connection<T>>
{
public:
    connection() {}
    virtual ~connection() {}
public:
    // Called by clients.
    bool ConnectToServer();
    // Called by both clients and servers.
    bool Disconnect();
    // Is the connection still active?
    bool IsConnected() const;
public:
    // Send a message to the remote endpoint.
    void Send(const message<T>& msg);
protected:
    // Responsible for the ASIO stuff.
    asio::ip::tcp::socket m_socket;
    // This context is shared with the whole asio instance.
    // Provided by the client or server interface.
    asio::io_context m_asioContext;
    // This queue holds all messages to be sent to the remote side.
    thread_safe_queue<T> m_qMessagesOut;
    // This queue holds all messages that have been received from the remote side.
    // Note it is a reference as the "owner" of this connection is expected to provide a queue.
    // Provided by the client or server interface.
    thread_safe_queue<owned_message<T>>& m_qMessagesIn;
};

} // namespace net