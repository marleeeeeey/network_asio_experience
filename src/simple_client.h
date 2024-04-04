#pragma once
#include "net_client.h"

enum class CustomMsgTypes : uint32_t
{
    FireBullet,
    MovePlayer
};

class CustomClient : public net::client_interface<CustomMsgTypes>
{
public:
    bool FireBullet(float x, float y)
    {
        net::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::FireBullet;
        msg << x << y;
        Send(msg);
        return true;
    }
};

void StartSimpleClient();

void StartCustomClient();