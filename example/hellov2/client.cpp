#include <stdio.h>
#include <unistd.h>
#include "maid/channel.h"
#include "maid/channel_factory.h"
#include "maid/channel_pool.h"
#include "maid/controller.h"
#include "maid/controller.pb.h"
#include "maid/closure.h"
#include "hello.pb.h"

#define REQUESTS 10000

using maid::Channel;
using maid::Controller;

static int32_t count = 0;

class Closure : public google::protobuf::Closure
{
public:
    Closure(maid::Controller* controller, maid::example::HelloRequest* request, maid::example::HelloResponse* response)
        :response_(response),
        request_(request),
        controller_(controller)
    {
    }

    void Run()
    {
        ++count;
        if (controller_->IsCanceled()) {
            controller_->proto().PrintDebugString();
        } else if (controller_->Failed()) {
            controller_->proto().PrintDebugString();
        } else {
            response_->PrintDebugString();
        }

        if(count >= REQUESTS){
            printf("count:%d\n", count);
            uv_stop(uv_default_loop());
        }

        delete request_;
        delete response_;
        delete controller_;
    }

private:
    maid::Controller* controller_;
    maid::example::HelloRequest* request_;
    maid::example::HelloResponse* response_;
};


int main()
{
    maid::ChannelPool* pool = new maid::ChannelPool();
    maid::Connector connector(NULL, NULL, pool);
    connector.Connect("0.0.0.0", 5555);
    for(int32_t i = 0; i < REQUESTS; i++){
        uv_run(uv_default_loop(), UV_RUN_NOWAIT);
        Controller* controller = new Controller();
        maid::example::HelloRequest* request = new maid::example::HelloRequest();
        request->set_message("hello");
        maid::example::HelloResponse* response = new maid::example::HelloResponse();
        Closure* closure = new Closure(controller, request, response);

        maid::example::HelloService_Stub stub(pool->channel());
        stub.Hello(controller, request, response, closure);
    }

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    connector.Close();

    printf("REQUESTS: %d\n", REQUESTS);

    sleep(5);
}
