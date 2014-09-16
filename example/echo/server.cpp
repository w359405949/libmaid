#include <stdio.h>
#include "maid.h"
#include "echo.pb.h"

int count = 0;

class EchoServiceImpl: public EchoService
{
public:
    void Echo(google::protobuf::RpcController* controller,
            const EchoRequest* request,
            EchoResponse* response,
            google::protobuf::Closure* done)
    {
        //printf("==========\n%s\n", request->DebugString().c_str());
        //printf("count:%d\n", ++count);
        response->set_message(request->message());
        done->Run();
    }
};


int main()
{
    maid::channel::Channel* channel = new maid::channel::Channel(uv_default_loop());
    EchoService* echo = new EchoServiceImpl();
    channel->AppendService(echo);
    channel->Listen("0.0.0.0", 8888);
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
