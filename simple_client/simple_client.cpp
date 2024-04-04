#include <cassert>
#include <my_cpp_utils/logger.h>
#include <net_common/net_client.h>
#include <net_common/net_message.h>

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

int main()
{
    utils::Logger::Init("logs/net_client.log", spdlog::level::trace);

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

void StartCustomClient()
{
    CustomClient c;
    c.Connect("asdf.com", 60000);
    c.FireBullet(1.0f, 2.0f);
}