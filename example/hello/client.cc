#include <stdio.h>
#include <google/protobuf/empty.pb.h>
#include "maid/base.h"
#include "maid/controller.h"
#include "hello.pb.h"
#define REQUESTS 1000

static int32_t count = 0;

class Closure : public google::protobuf::Closure
{
public:
    Closure(maid::Controller* controller, maid::example::HelloRequest* request, maid::example::HelloResponse* response, maid::TcpClient* client)
        :response_(response),
        request_(request),
        controller_(controller),
        client_(client)
    {
    }

    void Run()
    {
        ++count;
        printf("%d:client:%s, server:%s\n", count, request_->message().c_str(), response_->message().c_str());
        if(count >= REQUESTS){
            client_->Close();
        }

        delete request_;
        delete response_;
        delete controller_;
    }

private:
    maid::Controller* controller_;
    maid::example::HelloRequest* request_;
    maid::example::HelloResponse* response_;
    maid::TcpClient* client_;
};


class EmptyClosure : public google::protobuf::Closure
{
public:
    EmptyClosure(maid::Controller* controller, maid::example::HelloRequest* request, google::protobuf::Empty* response, maid::TcpClient* client)
        :response_(response),
        request_(request),
        controller_(controller),
        client_(client)
    {
    }

    void Run()
    {
        ++count;
        printf("%d:client:%s\n", count, request_->message().c_str());
        if(count >= REQUESTS){
            client_->Close();
        }

        delete request_;
        delete response_;
        delete controller_;
    }

private:
    maid::Controller* controller_;
    maid::example::HelloRequest* request_;
    google::protobuf::Empty* response_;
    maid::TcpClient* client_;
};


class HelloServiceImpl: public maid::example::HelloService
{
public:
    void Hello(google::protobuf::RpcController* controller,
            const maid::example::HelloRequest* request,
            maid::example::HelloResponse* response,
            google::protobuf::Closure* done) override
    {
        response->set_message("welcome to client");
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
};



int main()
{
    maid::TcpClient* client = new maid::TcpClient(uv_default_loop());
    client->InsertService(new HelloServiceImpl());
    //client->Connect("127.0.0.1", 5555);
    client->Connect("112.124.111.91", 5555);
    for(int32_t i = 0; i < REQUESTS / 2; i++){
        maid::Controller* controller = new maid::Controller();
        maid::example::HelloRequest* request = new maid::example::HelloRequest();
        request->set_message("hello");
        maid::example::HelloResponse* response = new maid::example::HelloResponse();
        Closure* closure = new Closure(controller, request, response, client);

        maid::example::HelloService_Stub stub(client->channel());
        stub.Hello(controller, request, response, closure);
        client->Update();
    }

    for(int32_t i = 0; i < REQUESTS / 2; i++) {
        maid::Controller* controller = new maid::Controller();
        maid::example::HelloRequest* request = new maid::example::HelloRequest();
        request->set_message("empty");
        google::protobuf::Empty* response = new google::protobuf::Empty();
        EmptyClosure* closure = new EmptyClosure(controller, request, response, client);

        maid::example::HelloService_Stub stub(client->channel());
        stub.HelloNotify(controller, request, response, closure);
        client->Update();
    }

    client->ServeForever();
}
