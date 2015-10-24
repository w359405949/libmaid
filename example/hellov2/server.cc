#include <stdio.h>
#include "maid/channel_factory.h"
#include "maid/controller.h"
#include "maid/channel.h"
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
        //printf("receive something\n");

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
        //maid::example::HelloRequest* req = new maid::example::HelloRequest();
        //req->set_message("request from libmaid");
        //stub->HelloNotify(con, req, NULL, NULL);
        //request->PrintDebugString();
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
        printf("hello notify:%s\n", request->DebugString().c_str());

        //int64_t connection_id = ((maid::Controller*)controller)->connection_id();
        //maid::Controller* con = new maid::Controller();
        //con->set_connection_id(connection_id);

        //maid::example::HelloRequest* req = new maid::example::HelloRequest();
        //req->set_message("request from libmaid");

        //maid::example::HelloResponse* res = new maid::example::HelloResponse();
        //MockClosure* stub_done = new MockClosure(con, req, res);
        //stub->HelloRpc(con, req, res, stub_done);

        done->Run();
    }

private:
};

void OnCtrlC(uv_signal_t* handle, int statu)
{
    maid::Acceptor* acceptor = (maid::Acceptor*)handle->data;

    printf("ctrl_c pressed:\n");
    acceptor->Close();
}

void DisconnectCallback(int64_t connection_id)
{
    printf("disconnected:%lld\n", connection_id);
    if (connection_id == 0) {
        uv_stop(uv_default_loop());
    }
}

void ConnectedCallback(int64_t connection_id)
{
    printf("connected:%lld\n", connection_id);
}

int main()
{
    uv_signal_t ctrl_c;
    uv_signal_init(uv_default_loop(), &ctrl_c);
    uv_signal_start(&ctrl_c, OnCtrlC, SIGINT);

    maid::LocalMapRepoChannel* router = new maid::LocalMapRepoChannel();
    router->Insert(new HelloServiceImpl());
    maid::Acceptor acceptor(uv_default_loop(), router);
    acceptor.AddDisconnectedCallback(DisconnectCallback);
    ctrl_c.data = &acceptor;
    acceptor.Listen("0.0.0.0", 5555);
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());
}
