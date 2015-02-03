#include <stdio.h>
#include "maid/channel_factory.h"
#include "maid/channel_pool.h"
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
        //maid::example::HelloRequest* req = new maid::example::HelloRequest();
        //req->set_message("request from libmaid");
        //stub->HelloNotify(con, req, NULL, NULL);
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


int main()
{
    maid::LocalMapRepoChannel* repo = new maid::LocalMapRepoChannel();
    maid::ChannelPool::generated_pool()->AddChannel(repo);
    repo->Insert(new HelloServiceImpl());
    maid::Acceptor acceptor(repo, NULL, NULL);
    acceptor.Listen("0.0.0.0", 5555);
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    acceptor.Close();
}
