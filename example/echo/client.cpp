#include <iostream>
#include <stdio.h>
#include "maid.h"
#include "echo.pb.h"

using maid::channel::Channel;
using maid::controller::Controller;

class Closure : public google::protobuf::Closure
{
public:
    Closure(google::protobuf::Message* response)
        :response_(response)
    {
    }
    void Run()
    {
        printf("%s\n", response_->DebugString().c_str());
    }

private:
    google::protobuf::Message* response_;
};


int main()
{
    Channel* channel = new Channel(EV_DEFAULT);
    int32_t fd = channel->Connect("127.0.0.1", 8888);
    Controller* controller = new Controller();
    controller->get_meta_data().set_fd(fd);
    EchoRequest* request = new EchoRequest();
    EchoResponse* response = new EchoResponse();
    Closure* closure = new Closure(response);
    EchoService_Stub* stub = new EchoService_Stub(channel);

    while(1){
        request->set_message("hello");
        stub->Echo(controller, request, response, closure);
        ev_run(EV_DEFAULT, 1);
    }
}
