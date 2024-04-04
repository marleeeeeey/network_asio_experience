#include <my_cpp_utils/logger.h>
#include <net_common/net_server.h>

enum class CustomMsgTypes : uint32_t
{
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage,
};

class CustomServer : public net::server_interface<CustomMsgTypes>
{
public:
    CustomServer(uint16_t port) : net::server_interface<CustomMsgTypes>(port) {}
protected:
    virtual bool OnClientConnect(std::shared_ptr<net::connection<CustomMsgTypes>> client)
    {
        // net::message<CustomMsgTypes> msg;
        // msg.header.id = CustomMsgTypes::ServerAccept;
        // client->Send(msg);
        // return true;
        return true;
    }

    virtual void OnClientDisconnect(std::shared_ptr<net::connection<CustomMsgTypes>> client)
    {
        MY_LOG_FMT(info, "Removing client [{}]", client->GetID());
    }

    virtual void OnMessage(std::shared_ptr<net::connection<CustomMsgTypes>> client, net::message<CustomMsgTypes>& msg)
    {
        switch (msg.header.id)
        {
        case CustomMsgTypes::ServerPing:
            {
                MY_LOG(info, "Ping from client");
                // Simply bounce message back to client.
                client->Send(msg);
                break;
            }
        case CustomMsgTypes::MessageAll:
            {
                // MY_LOG(info, "Message all from client");
                // net::message<CustomMsgTypes> msg;
                // msg.header.id = CustomMsgTypes::ServerMessage;
                // msg << client->GetID();
                // MessageAllClients(msg, client);
                break;
            }
        default:
            MY_LOG(error, "Unrecognized message type");
            break;
        }
    }
};

int main()
{
    utils::Logger::Init("logs/net_server.log", spdlog::level::trace);

    CustomServer server(60000);

    server.Start();

    while (1)
    {
        server.Update();
    }

    return 0;
}