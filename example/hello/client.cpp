#include <stdio.h>
#include "maid.h"
#include "hello.pb.h"

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
    Controller* controller = new Controller(EV_DEFAULT);
    controller->get_meta_data().set_fd(fd);
    HelloRequest* request = new HelloRequest();
    request->set_message("hello");
    HelloResponse* response = new HelloResponse();
    Closure* closure = new Closure(response);

    HelloService_Stub* stub = new HelloService_Stub(channel);
    stub->Hello(controller, request, response, closure);

    ev_run(EV_DEFAULT, 0);
}
