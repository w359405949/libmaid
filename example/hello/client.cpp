#include <stdio.h>
#include "maid.h"
#include "hello.pb.h"
#define MAX_CONNECTION 100000

using maid::channel::Channel;
using maid::controller::Controller;

static int32_t count = 0;

class Closure : public maid::closure::Closure
{
public:
    void Run()
    {
        if (!controller()->Failed()) {
            ++count;
            printf("count:%d\n", count);
        }
        if(count + 1 >= MAX_CONNECTION){
            uv_stop(uv_default_loop());
        }
        printf("%s\n", ((HelloResponse*)response())->message().c_str());
    }

private:
    google::protobuf::Message* response_;
};


int main()
{
    Channel* channel = new Channel(uv_default_loop());
    channel->Connect("127.0.0.1", 8888, true);
    for(int32_t i = 0; i < MAX_CONNECTION; i++){
        uv_run(uv_default_loop(), UV_RUN_ONCE);
        Controller* controller = new Controller();
        HelloRequest* request = new HelloRequest();
        request->set_message("hello");
        HelloResponse* response = new HelloResponse();
        Closure* closure = new Closure();

        HelloService_Stub* stub = new HelloService_Stub(channel);
        stub->Hello(controller, request, response, closure);
    }
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

}
