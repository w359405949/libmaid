#include <stdio.h>
#include "maid.h"
#include "hello.pb.h"

class HelloServiceImpl: public HelloService
{
public:
    void Hello(google::protobuf::RpcController* controller,
            HelloRequest* request,
            HelloResponse* response,
            google::protobuf::Closure* done)
    {
        printf("%s\n", request->message().c_str());
        response->set_message("welcome to libmaid");
        done->Run();
    }
};


int main()
{
    maid::channel::Channel* channel = new maid::channel::Channel(EV_DEFAULT);
    channel->Listen("0.0.0.0", 8888);
    ev_run(EV_DEFAULT, 0);
}
