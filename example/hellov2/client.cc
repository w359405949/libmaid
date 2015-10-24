#include <stdio.h>
#include <unistd.h>
#include "maid/channel.h"
#include "maid/channel_factory.h"
#include "maid/controller.h"
#include "maid/controller.pb.h"
#include "hello.pb.h"

#define REQUESTS 1000000

using maid::Channel;
using maid::Controller;

static int32_t count = 0;

class Closure : public google::protobuf::Closure
{
public:
    Closure(maid::Controller* controller, maid::example::HelloRequest* request, maid::example::HelloResponse* response, maid::Connector* connector)
        :connector_(connector),
        response_(response),
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

        if(count == REQUESTS){
            printf("count:%d\n", count);
            connector_->Close();
        }

        delete request_;
        delete response_;
        delete controller_;
    }

private:
    maid::Connector* connector_;
    maid::Controller* controller_;
    maid::example::HelloRequest* request_;
    maid::example::HelloResponse* response_;
};


void OnCtrlC(uv_signal_t* handle, int statu)
{
    maid::Connector* connector = (maid::Connector*)handle->data;
    printf("ctrl_c pressed:\n");
    connector->Close();
}

void DisconnectCallback(int64_t connection_id)
{
    printf("disconnected:%lld\n", connection_id);
    if (connection_id == 0) {
        uv_stop(uv_default_loop());
    }
}

int main()
{
    uv_signal_t ctrl_c;
    uv_signal_init(uv_default_loop(), &ctrl_c);
    uv_signal_start(&ctrl_c, OnCtrlC, SIGINT);


    maid::Connector connector(uv_default_loop(), NULL);
    ctrl_c.data = &connector;
    connector.AddDisconnectedCallback(DisconnectCallback);
    connector.Connect("0.0.0.0", 5555);
    for(int32_t i = 0; i < REQUESTS; i++){
        //sleep(1);
        uv_run(uv_default_loop(), UV_RUN_NOWAIT);
        Controller* controller = new Controller();
        maid::example::HelloRequest* request = new maid::example::HelloRequest();
        request->set_message("hello");
        maid::example::HelloResponse* response = new maid::example::HelloResponse();
        Closure* closure = new Closure(controller, request, response, &connector);

        maid::example::HelloService_Stub stub(connector.channel());
        stub.Hello(controller, request, response, closure);
    }

    printf("waiting:\n");
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    uv_loop_close(uv_default_loop());

    printf("REQUESTS: %d\n", REQUESTS);
}
