#pragma once
#include "net_connection.h"
#include "net_message.h"
#include "net_thread_safe_queue.h"
#include <asio.hpp>
#include <asio/ip/tcp.hpp>
#include <my_cpp_utils/logger.h>

namespace net
{
// Responsible for setting up the ASIO and setting up the connection.
// Also access point to talk to the server.
template <typename T>
class client_interface
{
public:
    client_interface()
    {
        // Initialize the socket with the ASIO context, so it can do stuff.
    }
    virtual ~client_interface()
    {
        // If the client is destroyed, always try to disconnect gracefully.
        Disconnect();
    }

    // Connect to the server at a given IP address and port.
    bool Connect(const std::string& host, const uint16_t port)
    {
        try
        {
            // Resolve the IP address.
            asio::ip::tcp::resolver resolver(m_context);
            auto endpoints = resolver.resolve(host, std::to_string(port));

            // Create a connection.
            m_connection = std::make_unique<connection<T>>(
                connection<T>::owner::client, m_context, asio::ip::tcp::socket(m_context), m_qMessagesIn);

            // Tell the connection object to connect to the server.
            m_connection->ConnectToServer(endpoints);

            // Start the ASIO context thread.
            thrContext = std::thread([this]() { m_context.run(); });
        }
        catch (std::exception& e)
        {
            MY_LOG(error, "Client Exception: {}", e.what());
            return false;
        }

        return false;
    }

    // Disconnect from the server.
    void Disconnect()
    {
        // If connection exists, and it's connected then...
        if (IsConnected())
        {
            // ... disconnect from server gracefully.
            m_connection->Disconnect();
        }

        // Stop the ASIO context.
        m_context.stop();

        // Tidy up the context thread.
        if (thrContext.joinable())
            thrContext.join();

        // Destroy the connection object.
        m_connection.release();
    }

    bool IsConnected() const
    {
        if (m_connection)
            return m_connection->IsConnected();
        else
            return false;
    }

    // Retrieve queue of messages from the server.
    thread_safe_queue<owned_message<T>>& Incoming() { return m_qMessagesIn; }

    // Send message to the server.
    void Send(const message<T>& msg)
    {
        if (IsConnected())
            m_connection->Send(msg);
    }
protected:
    // ASIO context handles the data transfer...
    asio::io_context m_context;
    // ...but needs a thread of execution to operate.
    std::thread thrContext;
    // Each client has a single instance of the "connection" class.
    std::unique_ptr<connection<T>> m_connection;
private:
    // This is thread safe queue of incoming messages from the server.
    thread_safe_queue<owned_message<T>> m_qMessagesIn;
};
} // namespace net