#include <asio.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <my_cpp_utils/logger.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    utils::Logger::Init("logs/network_asio_experience.log", spdlog::level::trace);
    MY_LOG(info, "Starting network_asio_experience application");

    // Error code for asio operations. Exception can also be used.
    asio::error_code ec;

    // Hide all platform-specific requirements.
    asio::io_context context;

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
        std::string request =
            "GET /index.html HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Connection: close\r\n\r\n";

        // Send some data to the server.
        // write_some means write as much as you can.
        // asio::buffer is a container like array of bytes.
        socket.write_some(asio::buffer(request.data(), request.size()), ec);
        if (ec)
            MY_LOG_FMT(error, "Failed to send request: {}", ec.message());

        // This will hold the response.
        socket.wait(socket.wait_read);

        // TODO There may be more data to read. So we should to solve this problem.
        size_t bytes = socket.available();
        MY_LOG_FMT(info, "Bytes available: {}", bytes);

        if (bytes > 0)
        {
            // Syncronously read some data from the server.
            std::vector<char> buffer(bytes);
            socket.read_some(asio::buffer(buffer.data(), buffer.size()), ec);

            // Convert the buffer to a string.
            std::string response(buffer.begin(), buffer.end());
            MY_LOG(info, response);
        }
    }

    MY_LOG(info, "Closing network_asio_experience application");
}