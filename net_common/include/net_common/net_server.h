#pragma once
#include "net_connection.h"
#include "net_message.h"
#include <cstdint>
#include <exception>
#include <fmt/chrono.h>
#include <my_cpp_utils/logger.h>

namespace net
{
template <typename T>
class server_interface
{
public:
    server_interface(uint16_t port) : m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
    {}
    virtual ~server_interface() { Stop(); }

    bool Start()
    {
        try
        {
            // The order of these methods is important.
            // We should start waiting for a connection first.
            // Then we should start the thread context.
            // In other cases, the thread context may stop because there is no work to do.
            WaitForClientConnection();
            m_threadContext = std::thread([this]() { m_asioContext.run(); });
        }
        catch (std::exception& e)
        {
            MY_LOG_FMT(error, "[server_interface] Exception: {}", e.what());
            return false;
        }

        MY_LOG(info, "[server_interface] Started!");
        return true;
    }

    void Stop()
    {
        // Request the context to close.
        m_asioContext.stop();

        // Tidy up the context thread.
        if (m_threadContext.joinable())
            m_threadContext.join();

        // Inform someone, anybody, if they care...
        MY_LOG(info, "[server_interface] Stopped!");
    }
private:
    // ASYNC - Instruct ASIO to wait for connection.
    void WaitForClientConnection()
    {
        m_asioAcceptor.async_accept(
            [this](std::error_code ec, asio::ip::tcp::socket socket)
            {
                if (!ec)
                {
                    MY_LOG_FMT(
                        info, "[server_interface] New Connection: {}", socket.remote_endpoint().address().to_string());

                    // Create a new connection to handle this client and start waiting for more connections.
                    // Server and client behave are different. That's why we need to specify the owner as server.
                    // Use one queue for all connections(clients).
                    std::shared_ptr<connection<T>> newconn = std::make_shared<connection<T>>(
                        connection<T>::owner::server, m_asioContext, std::move(socket), m_qMessagesIn);

                    if (OnClientConnect(newconn))
                    {
                        // Connection allowed, so add to container of new connections.
                        m_deqConnections.push_back(std::move(newconn));
                        m_deqConnections.back()->ConnectToClient(nIDCounter++);

                        MY_LOG_FMT(
                            info, "[server_interface] Connection Approved. ID: {}", m_deqConnections.back()->GetID());
                    }
                    else
                    {
                        MY_LOG(info, "[server_interface] Connection Denied");
                    }
                }
                else
                {
                    MY_LOG_FMT(error, "[server_interface] New Connection Error: {}", ec.message());
                }

                // Prime the asio context with more work - again simply wait for another connection...
                WaitForClientConnection();
            });
    }
public:
    // Send a message to a specific client.
    void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg)
    {
        if (client && client->IsConnected())
        {
            client->Send(msg);
        }
        else
        {
            // If we couldn't communicate with the client then we may as well remove the client - it's dead.
            OnClientDisconnect(client);
            client.reset();

            // Then remove the dead client connection from the container.
            // In case of huge number of clients, we should use a more efficient data structure.
            m_deqConnections.erase(
                std::remove(m_deqConnections.begin(), m_deqConnections.end(), client), m_deqConnections.end());
        }
    }

    // Send a message to all clients.
    void MessageAllClients(const message<T>& msg, std::shared_ptr<connection<T>> pIgnoreClient = nullptr)
    {
        MY_LOG_FMT(
            info, "[server_interface::MessageAllClients] Sending message: ID {}, Size {}", msg.header.id,
            msg.header.size);

        // Optimization: This flag helps us to remove dead connections once we finish sending messages.
        bool bInvalidClientExists = false;

        for (auto& client : m_deqConnections)
        {
            // Check if the client is still connected.
            if (client && client->IsConnected())
            {
                // If the client is not the one we are ignoring, send the message.
                if (client != pIgnoreClient)
                    client->Send(msg);
            }
            else
            {
                // If we couldn't communicate with the client then we may as well remove the client - it's dead.
                OnClientDisconnect(client);
                client.reset();
                bInvalidClientExists = true;
            }
        }

        // Remove the dead client connections from the container. We should use a more efficient data structure anyway.
        if (bInvalidClientExists)
        {
            m_deqConnections.erase(
                std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr), m_deqConnections.end());
            MY_LOG(info, "[server_interface] Cleaned up dead connections");
        }
    }

    // It is allowed to user decide when is the most appropriate time to actually handle incoming messages.
    // nMaxMessages = -1 means "process all messages". This flag is used to restrict the number of messages to process
    // to prevent the server from being overwhelmed.
    void Update(size_t nMaxMessages = -1, bool bWait = false)
    {
        if (bWait)
            m_qMessagesIn.wait();

        size_t nMessageCount = 0;
        while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty())
        {
            // Grab the front message.
            auto msg = m_qMessagesIn.pop_front();

            // Handle the message.
            OnMessage(msg.remote, msg.msg);

            nMessageCount++;
        }
    }
protected:
    // Called when a client connects, you can veto the connection by returning false.
    // Other words this function is a filter of clients (connections).
    virtual bool OnClientConnect(std::shared_ptr<connection<T>> client) { return false; }

    // Called when a client appears to have disconnected.
    virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client)
    {
        // Remove the client from a game etc...
    }

    // Called when a message arrives.
    virtual void OnMessage(std::shared_ptr<connection<T>> client, message<T>& msg) {}
protected:
    // Thread safe queue for incoming message packets.
    thread_safe_queue<owned_message<T>> m_qMessagesIn;

    // Container of active validated connections.
    std::deque<std::shared_ptr<connection<T>>> m_deqConnections;

    // Order of declaration is important - it is also the order of initialization.
    asio::io_context m_asioContext;
    std::thread m_threadContext;

    // Sockets hides into the asio details. We will only have a listener socket.
    asio::ip::tcp::acceptor m_asioAcceptor;

    // Clients will be identified in the system via an ID.
    // This number will be send to clients. This is more secure than sending the IP address.
    uint32_t nIDCounter = 10000;
};
} // namespace net