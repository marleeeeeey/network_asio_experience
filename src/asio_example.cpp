#include "asio_example.h"

#include <asio.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <iostream>
#include <my_cpp_utils/logger.h>
#include <vector>

void GrabSomeDataAsync(asio::ip::tcp::socket& socket)
{
    static std::vector<char> vBuffer(1 * 1024);

    // Request data from the server.
    socket.async_read_some(
        asio::buffer(vBuffer.data(), vBuffer.size()),
        [&socket](std::error_code ec, std::size_t length)
        {
            if (!ec)
            {
                std::cout << "\n\nRead " << length << " bytes\n\n";
                std::cout << std::string(vBuffer.data(), length) << std::endl;

                // It is not a recursive call. It is a new call.
                GrabSomeDataAsync(socket);
            }
        });
}

void StartAsioExample()
{
    // Error code for asio operations. Exception can also be used.
    asio::error_code ec;

    // Hide all platform-specific requirements.
    asio::io_context context;

    // Add fake work to the context to prevent it from returning.
    asio::io_context::work idleWork(context);

    // Start the context in the background.
    // Run returns when there is no more work to do - no more instructions to execute.
    std::thread thrContext = std::thread([&]() { context.run(); });

    // Get the address of somewhere we wish to connect to.
    // 93.184.216.34 = example.com
    // 51.38.81.49 = google.com
    asio::ip::tcp::endpoint endpoint(asio::ip::make_address("51.38.81.49", ec), 80);

    // Create a socket, the context will deliver the implementation.
    asio::ip::tcp::socket socket(context);

    // Tell socket to try and connect.
    (void)socket.connect(endpoint, ec);

    if (!ec)
    {
        MY_LOG(info, "Connected!");
    }
    else
    {
        MY_LOG_FMT(error, "Failed to connect to address: {}", ec.message());
    }

    if (socket.is_open())
    {
        // Start an asynchronous operation to read data.
        // Is is started before sending the request to prevent getting empty data.
        GrabSomeDataAsync(socket);

        std::string request =
            "GET /index.html HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Connection: close\r\n\r\n";

        // Send request to the server.
        // write_some means write as much as you can.
        // asio::buffer is a container like array of bytes.
        socket.write_some(asio::buffer(request.data(), request.size()), ec);
        if (ec)
            MY_LOG_FMT(error, "Failed to send request: {}", ec.message());
    }

    // Delay the closing of the application.
    std::this_thread::sleep_for(std::chrono::seconds(5));
}
