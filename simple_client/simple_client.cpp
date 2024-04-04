#include <my_cpp_utils/logger.h>
#include <net_common/net_client.h>
#include <net_common/net_message.h>

enum class CustomMsgTypes : uint32_t
{
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage,
};

class CustomClient : public net::client_interface<CustomMsgTypes>
{
public:
    CustomClient() {}

    void PingServer()
    {
        net::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::ServerPing;

        // Measure round trip time.
        // Caution with this ...
        std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
        msg << timeNow;
        Send(msg);
    }
};

int main()
{
    utils::Logger::Init("logs/simple_client.log", spdlog::level::trace);

    CustomClient c;
    c.Connect("127.0.0.1", 60000);

    bool key[3] = {false, false, false};
    bool old_key[3] = {false, false, false};

    bool bQuit = false;
    while (!bQuit)
    {
        // Windows message handling.
        if (GetForegroundWindow() == GetConsoleWindow())
        {
            key[0] = GetAsyncKeyState('1') & 0x8000;
            key[1] = GetAsyncKeyState('2') & 0x8000;
            key[2] = GetAsyncKeyState('3') & 0x8000;
        }

        if (key[0] && !old_key[0])
        {
            c.PingServer();
        }
        if (key[1] && !old_key[1])
        {
            net::message<CustomMsgTypes> msg;
            msg.header.id = CustomMsgTypes::MessageAll;
            c.Send(msg);
        }
        if (key[2] && !old_key[2])
        {
            bQuit = true;
        }
    }

    // If the client still connected.
    if (c.IsConnected())
    {
        // If client has incoming messages.
        if (!c.Incoming().empty())
        {
            auto msg = c.Incoming().pop_front().msg;

            switch (msg.header.id)
            {
            case CustomMsgTypes::ServerAccept:
                {
                    MY_LOG(info, "Server accepted connection");
                    break;
                }
            case CustomMsgTypes::ServerDeny:
                {
                    MY_LOG(info, "Server denied connection");
                    break;
                }
            case CustomMsgTypes::ServerPing:
                {
                    MY_LOG(info, "Server pinged us");

                    // Measure round trip time in seconds.
                    std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
                    std::chrono::system_clock::time_point timeThen;
                    msg >> timeThen;
                    MY_LOG_FMT(info, "Ping: {}", std::chrono::duration<double>(timeNow - timeThen).count());

                    break;
                }
            case CustomMsgTypes::ServerMessage:
                {
                    MY_LOG(info, "Server has sent a message");
                    break;
                }
            case CustomMsgTypes::MessageAll:
                break;
            }
        }
    }
    else
    {
        MY_LOG(error, "Server Down");
        bQuit = true;
    }
}
