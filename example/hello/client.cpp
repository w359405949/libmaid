#include <stdio.h>
#include "maid.h"
#include "hello.pb.h"
#define MAX_CONNECTION 1000

using maid::channel::Channel;
using maid::controller::Controller;



static int32_t count = 0;

class Closure : public google::protobuf::Closure
{
public:
    Closure(google::protobuf::Message* response)
        :response_(response)
    {
    }
    void Run()
    {
        ++count;
        printf("count:%d\n", count);
        if(count + 1 >= MAX_CONNECTION){
            ev_break(EV_DEFAULT, EVBREAK_ALL);
        }
        //printf("%s\n", response_->DebugString().c_str());
    }

private:
    google::protobuf::Message* response_;
};


int main()
{
    Channel* channel = new Channel(EV_DEFAULT);
    for(int32_t i = 0; i < MAX_CONNECTION; i++){
        //printf("connection:%d\n", i);
        int32_t fd = channel->Connect("127.0.0.1", 8888);
        Controller* controller = new Controller(EV_DEFAULT);
        controller->set_fd(fd);
        HelloRequest* request = new HelloRequest();
        request->set_message("hello");
        HelloResponse* response = new HelloResponse();
        Closure* closure = new Closure(response);

        HelloService_Stub* stub = new HelloService_Stub(channel);
        stub->Hello(controller, request, response, closure);
    }
    ev_run(EV_DEFAULT, 0);

}
