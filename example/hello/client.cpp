#include <stdio.h>
#include "maid.h"
#include "hello.pb.h"
#define MAX_CONNECTION 100000

using maid::Channel;
using maid::Controller;

static int32_t count = 0;

class Closure : public google::protobuf::Closure
{
public:
    Closure(maid::Controller* controller, HelloRequest* request, HelloResponse* response)
        :response_(response),
        request_(request),
        controller_(controller)
    {
    }

    void Run()
    {
        if (!controller_->Failed()) {
            ++count;
            printf("count:%d\n", count);
        }
        if(count + 1 >= MAX_CONNECTION){
            uv_stop(uv_default_loop());
        }
        printf("%s\n", response_->message().c_str());

        delete request_;
        delete response_;
        delete controller_;
    }

private:
    maid::Controller* controller_;
    HelloRequest* request_;
    HelloResponse* response_;
};


int main()
{
    Channel* channel = new Channel();
    channel->Connect("127.0.0.1", 8888, true);
    for(int32_t i = 0; i < MAX_CONNECTION; i++){
        Controller* controller = new Controller();
        HelloRequest* request = new HelloRequest();
        request->set_message("hello");
        HelloResponse* response = new HelloResponse();
        Closure* closure = new Closure(controller, request, response);

        HelloService_Stub* stub = new HelloService_Stub(channel);
        stub->Hello(controller, request, response, closure);
        channel->Update();
    }
    channel->ServeForever();
}
