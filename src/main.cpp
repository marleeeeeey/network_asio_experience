#include <my_cpp_utils/logger.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    utils::Logger::Init("logs/cpp_project_template.log", spdlog::level::trace);
    MY_LOG(info, "Starting cpp_project_template application");
    MY_LOG(info, "Closing cpp_project_template application");
}