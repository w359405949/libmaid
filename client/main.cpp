#include <ev.h>

#include "hello.pb.h"
#include "channel.h"
#include "controller.h"

using maid::channel::Channel;
using maid::controller::Controller;

int main()
{
    struct ev_loop* loop = EV_DEFAULT;
    Channel *channel = new Channel(loop, "192.168.85.58", 5000);
    EchoService_Stub *service = new EchoService_Stub(channel);
    Controller *controller = new Controller();
    Say *say = new Say();
    service->Echo(controller, say, NULL, NULL);

    ev_run(loop, 0);
}
