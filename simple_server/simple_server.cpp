#include <my_cpp_utils/logger.h>
#include <net_common/net_server.h>
#include <simple_common/custom_msg_type.h>
#include <simple_common/settings.h>

class CustomServer : public net::server_interface<CustomMsgTypes>
{
public:
    CustomServer(uint16_t port) : net::server_interface<CustomMsgTypes>(port) {}
protected:
    virtual bool OnClientConnect(std::shared_ptr<net::connection<CustomMsgTypes>> client)
    {
        net::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::ServerAccept;
        client->Send(msg);
        return true;
    }

    virtual void OnClientDisconnect(std::shared_ptr<net::connection<CustomMsgTypes>> client)
    {
        MY_LOG_FMT(info, "[CustomServer::OnClientDisconnect] Client {} disconnected", client->GetID());
    }

    virtual void OnMessage(std::shared_ptr<net::connection<CustomMsgTypes>> client, net::message<CustomMsgTypes>& msg)
    {
        switch (msg.header.id)
        {
        case CustomMsgTypes::ServerPing:
            {
                MY_LOG_FMT(info, "[CustomServer::OnMessage] ServerPing received from client {}", client->GetID());
                client->Send(msg);
                break;
            }
        case CustomMsgTypes::MessageAll:
            {
                MY_LOG_FMT(info, "[CustomServer::OnMessage] MessageAll received from client {}", client->GetID());
                net::message<CustomMsgTypes> msg;
                msg.header.id = CustomMsgTypes::ServerMessage;
                msg << client->GetID();
                MessageAllClients(msg, client);
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
    utils::Logger::Init("logs/simple_server.log", spdlog::level::info);

    CustomServer server(settings::defaultPort);

    server.Start();

    while (true)
    {
        server.Update(-1, true);
    }

    return 0;
}