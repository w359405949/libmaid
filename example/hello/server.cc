#include <stdio.h>
#include "maid/base.h"
#include "maid/controller.h"
#include "hello.pb.h"

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
        //maid::Controller* con = new maid::Controller();
        //maid::example::HelloRequest* req= new maid::example::HelloRequest();
        //google::protobuf::Empty* res = new google::protobuf::Empty();
        //req->set_message("empty");
        //MockClosure* closure = new MockClosure(con, req, res);
        //stub.HelloNotify(con, req, res, closure);

        request->PrintDebugString();
        response->set_message("welcome to server");
        done->Run();
    }

    void HelloNotify(google::protobuf::RpcController* controller,
            const maid::example::HelloRequest* request,
            google::protobuf::Empty* response,
            google::protobuf::Closure* done) override
    {
        request->PrintDebugString();
        done->Run();
    }

private:
    maid::TcpServer* server_;
};


void OnCtrlC(uv_signal_t* handle, int statu)
{
    printf("ctrl_c pressed:\n");

    maid::TcpServer* server = (maid::TcpServer*)handle->data;
    server->Close();
}


int main()
{
    maid::TcpServer* server = new maid::TcpServer();

    uv_signal_t ctrl_c;
    ctrl_c.data = server;
    uv_signal_init(server->mutable_loop(), &ctrl_c);
    uv_signal_start(&ctrl_c, OnCtrlC, SIGINT);

    server->InsertService(new HelloServiceImpl(server));
    server->Listen("0.0.0.0", 5555);
    server->ServeForever();

    delete server;
}
