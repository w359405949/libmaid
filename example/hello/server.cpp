#include <stdio.h>
#include "maid/uv_hook.h"
#include "maid/base.h"
#include "maid/controller.h"
#include "maid/middleware/log_middleware.h"
#include "hello.pb.h"

class MockClosure : public google::protobuf::Closure
{
public:
    MockClosure(maid::Controller* controller, google::protobuf::Message* request, google::protobuf::Message* response)
        :controller_(controller),
        request_(request),
        response_(response)
    {
    }

    void Run()
    {
        printf("receive something\n");

        delete controller_;
        delete request_;
        delete response_;
    }

private:
    maid::Controller* controller_;
    google::protobuf::Message* request_;
    google::protobuf::Message* response_;
};

class HelloServiceImpl: public maid::example::HelloService
{
public:
    void Hello(google::protobuf::RpcController* controller,
            const maid::example::HelloRequest* request,
            maid::example::HelloResponse* response,
            google::protobuf::Closure* done)
    {
        //printf("transmit_id:%d, %s\n", ((maid::Controller*)controller)->meta_data().transmit_id(), request->message().c_str());

        request->PrintDebugString();
        response->set_message("welcome to libmaid");
        done->Run();
    }

    void HelloRpc(google::protobuf::RpcController* controller,
            const maid::example::HelloRequest* request,
            maid::example::HelloResponse* response,
            google::protobuf::Closure* done)
    {
        printf("hello rpc:%s\n", request->DebugString().c_str());

        response->set_message("helle rpc");
        done->Run();
    }

    void HelloNotify(google::protobuf::RpcController* controller,
            const maid::example::HelloRequest* request,
            maid::example::HelloResponse* response,
            google::protobuf::Closure* done)
    {
        done->Run();
    }

private:
};


int main()
{
    maid::TcpServer* server = new maid::TcpServer();
    maid::example::HelloService* hello = new HelloServiceImpl();
    server->InsertService(hello);
    server->AppendMiddleware(new maid::LogMiddleware());
    server->Listen("0.0.0.0", 5555);
    server->ServeForever();
}
