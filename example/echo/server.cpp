#include <stdio.h>
#include "maid.h"
#include "echo.pb.h"

class EchoServiceImpl: public EchoService
{
public:
    void Echo(google::protobuf::RpcController* controller,
            const EchoRequest* request,
            EchoResponse* response,
            google::protobuf::Closure* done)
    {
        response->set_message(request->message());
        done->Run();
    }
};


int main()
{
    maid::channel::Channel* channel = new maid::channel::Channel(EV_DEFAULT);
    EchoService* echo = new EchoServiceImpl();
    channel->AppendService(echo);
    channel->Listen("0.0.0.0", 8888);
    ev_run(EV_DEFAULT, 0);
}
