#include <stdio.h>
#include "maid/maid.h"
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

        int64_t connection_id = ((maid::Controller*)controller)->connection_id();
        maid::Controller* con = new maid::Controller();
        con->set_connection_id(connection_id);
        maid::example::HelloRequest* req = new maid::example::HelloRequest();
        req->set_message("request from libmaid");
        stub->HelloNotify(con, req, NULL, NULL);
        delete con;
        delete req;

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

        int64_t connection_id = ((maid::Controller*)controller)->connection_id();
        maid::Controller* con = new maid::Controller();
        con->set_connection_id(connection_id);

        maid::example::HelloRequest* req = new maid::example::HelloRequest();
        req->set_message("request from libmaid");

        maid::example::HelloResponse* res = new maid::example::HelloResponse();
        MockClosure* stub_done = new MockClosure(con, req, res);
        stub->HelloRpc(con, req, res, stub_done);

        done->Run();
    }


    HelloServiceImpl(maid::Channel* channel)
        :stub(NULL)
    {
        stub = new maid::example::HelloService_Stub(channel);
    }
private:
    maid::example::HelloService_Stub* stub;
};


int main()
{
    maid::Channel* channel = new maid::Channel();
    maid::example::HelloService* hello = new HelloServiceImpl(channel);
    channel->AppendService(hello);
    channel->Listen("0.0.0.0", 5555);
    channel->ServeForever();
}
