#include <ev.h>

#include "hello.pb.h"
#include "channel.h"
#include "controller.h"


int main()
{
    Channel *channel = new Channel("192.168.85.58", 5000);
    EchoService_Stub *service = new EchoService_Stub(channel);
    Controller *controller = new Controller();
    Say *say = new Say();
    service->Echo(controller, say, NULL, NULL);

    ev_run(EV_A_ 0);
}
