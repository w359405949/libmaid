#include <stdio.h>
#include "maid.h"
#include "hello.pb.h"

class HelloServiceImpl: public HelloService
{
public:
    void Hello(google::protobuf::RpcController* controller,
            const HelloRequest* request,
            HelloResponse* response,
            google::protobuf::Closure* done)
    {
        //printf("transmit_id:%d, %s\n", ((maid::Controller*)controller)->meta_data().transmit_id(), request->message().c_str());
        response->set_message("welcome to libmaid");
        done->Run();
    }
};


int main()
{
    maid::Channel* channel = new maid::Channel();
    HelloService* hello = new HelloServiceImpl();
    channel->AppendService(hello);
    channel->Listen("0.0.0.0", 8888);
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
