#include <stdio.h>
#include "maid/base.h"
#include "maid/controller.h"
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
    HelloServiceImpl(maid::TcpServer* server)
        :server_(server)
    {
    }

    void Hello(google::protobuf::RpcController* controller,
            const maid::example::HelloRequest* request,
            maid::example::HelloResponse* response,
            google::protobuf::Closure* done) override
    {
        maid::example::HelloService_Stub stub(server_->channel(google::protobuf::down_cast<maid::Controller*>(controller)->proto().connection_id()));
        maid::Controller* con = new maid::Controller();
        maid::example::HelloRequest* req= new maid::example::HelloRequest();
        google::protobuf::Empty* res = new google::protobuf::Empty();
        req->set_message("empty");
        MockClosure* closure = new MockClosure(con, req, res);
        stub.HelloNotify(con, req, res, closure);

        request->PrintDebugString();
        response->set_message("welcome to server");
        done->Run();
    }

    void HelloNotify(google::protobuf::RpcController* controller,
            const maid::example::HelloRequest* request,
            google::protobuf::Empty* response,
            google::protobuf::Closure* done) override
    {
        printf("channel:%lld\n", google::protobuf::down_cast<maid::Controller*>(controller)->proto().connection_id());
        request->PrintDebugString();
        done->Run();
    }

private:
    maid::TcpServer* server_;
};

int main()
{
    maid::TcpServer* server = new maid::TcpServer();
    server->InsertService(new HelloServiceImpl(server));
    server->Listen("0.0.0.0", 5555);
    server->ServeForever();
}
