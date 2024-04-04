#include <my_cpp_utils/logger.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    utils::Logger::Init("logs/network_asio_experience.log", spdlog::level::trace);
    MY_LOG(info, "Starting network_asio_experience application");
    MY_LOG(info, "Closing network_asio_experience application");
}