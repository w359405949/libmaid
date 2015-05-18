#include <stdio.h>
#include "maid/uv_hook.h"
#include "maid/base.h"
#include "maid/controller.h"
#include "maid/middleware/log_middleware.h"
#include "hello.pb.h"
#define REQUESTS 10000

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
        printf("%d:%s\n", count, response_->message().c_str());
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


int main()
{
    maid::TcpClient* client = new maid::TcpClient();
    client->AppendMiddleware(new maid::LogMiddleware());
    client->Connect("127.0.0.1", 5555);
    for(int32_t i = 0; i < REQUESTS; i++){
        maid::Controller* controller = new maid::Controller();
        maid::example::HelloRequest* request = new maid::example::HelloRequest();
        request->set_message("hello");
        maid::example::HelloResponse* response = new maid::example::HelloResponse();
        Closure* closure = new Closure(controller, request, response, client);

        maid::example::HelloService_Stub stub(client->channel());
        stub.Hello(controller, request, response, closure);
        client->Update();
    }
    client->ServeForever();

    uv_run(maid_default_loop(), UV_RUN_NOWAIT);
}
