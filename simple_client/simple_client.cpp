#include "sdl_RAII.h"
#include <my_cpp_utils/logger.h>
#include <my_cpp_utils/string_utils.h>
#include <net_common/net_client.h>
#include <net_common/net_message.h>
#include <simple_common/custom_msg_type.h>
#include <simple_common/settings.h>

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

        MY_LOG_FMT(
            debug, "[PingServer] Send message: ID {}, BodySizeInHeader {}, RealBodySize {}", msg.header.id,
            msg.header.size, msg.body.size());

        Send(msg);
    }

    void MessageAll()
    {
        net::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::MessageAll;

        MY_LOG_FMT(
            debug, "[MessageAll] Send message: ID {}, BodySizeInHeader {}, RealBodySize {}", msg.header.id,
            msg.header.size, msg.body.size());

        Send(msg);
    }
};

int SDL_main(int argv, char** args)
{
    utils::Logger::Init("logs/simple_client.log", spdlog::level::info);

    CustomClient c;
    c.Connect("127.0.0.1", settings::defaultPort);

    try
    {
        SDLApp app(SDL_INIT_VIDEO);
        SDLWindow window(
            "Press 1 to ping server, 2 to message all, 3 to quit", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640,
            480, SDL_WINDOW_SHOWN);

        bool quit = false;
        while (!quit)
        {
            SDL_Event e;
            while (SDL_PollEvent(&e) != 0)
            {
                if (e.type == SDL_QUIT)
                {
                    quit = true;
                }

                if (e.type == SDL_KEYDOWN)
                {
                    switch (e.key.keysym.sym)
                    {
                    case SDLK_1:
                        c.PingServer();
                        break;
                    case SDLK_2:
                        c.MessageAll();
                        break;
                    case SDLK_3:
                        quit = true;
                        break;
                    }
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
                            MY_LOG(info, "[SDL_main] Server accepted connection");
                            break;
                        }
                    case CustomMsgTypes::ServerDeny:
                        {
                            MY_LOG(info, "[SDL_main] Server denied connection");
                            break;
                        }
                    case CustomMsgTypes::ServerPing:
                        {
                            // Measure round trip time in seconds.
                            std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
                            std::chrono::system_clock::time_point timeThen;
                            msg >> timeThen;
                            auto durationSec = std::chrono::duration<double>(timeNow - timeThen).count();
                            MY_LOG_FMT(info, "[SDL_main] Recieved ping message. Round trip time: {}s", durationSec);
                            break;
                        }
                    case CustomMsgTypes::ServerMessage:
                        {
                            uint32_t clientID;
                            msg >> clientID;
                            MY_LOG_FMT(info, "[SDL_main] Recieved broadcast message from {}", clientID);
                            break;
                        }
                    case CustomMsgTypes::MessageAll:
                        break;
                    }
                }
            }
            else
            {
                MY_LOG(error, "[SDL_main] Server Down");
                quit = true;
            }
        }
    }
    catch (const std::runtime_error& e)
    {
        MY_LOG_FMT(error, "[SDL_main] Exception: {}", e.what());
        return -1;
    }

    return 0;
}
