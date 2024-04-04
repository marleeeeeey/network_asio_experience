#include "simple_client.h"
#include "net_message.h"
#include <cassert>
#include <cstdint>

enum class CustomMsgTypes : uint32_t
{
    FireBullet,
    MovePlayer
};

void StartSimpleClient()
{
    net::message<CustomMsgTypes> msg;
    msg.header.id = CustomMsgTypes::FireBullet;

    int a = 1;
    bool b = true;
    float c = 3.14159f;

    struct
    {
        float x;
        float y;
    } d[5];

    msg << a << b << c << d;

    a = 99;
    b = false;
    c = 42.0f;

    msg >> d >> c >> b >> a;

    assert(a == 1);
    assert(b == true);
    assert(c == 3.14159f);
}