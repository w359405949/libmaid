#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "maid/maid.h"
#include "echo.pb.h"

using maid::Channel;
using maid::Controller;

int count = 0;
int success_count = 0;

class Closure : public google::protobuf::Closure
{
public:
    Closure(maid::Controller* controller, EchoRequest* request, EchoResponse* response)
        :controller_(controller),
        request_(request),
        response_(response)
    {
    }

    void Run()
    {
        //printf("==========\n%s\n-------\n%s\n", controller()->meta_data().DebugString().c_str(), response()->DebugString().c_str());
        printf("%s\n", controller_->ErrorText().c_str());
        if (!controller_->Failed()) {
            //printf("success count:%d\n", ++success_count);
            count++;
        }
        //printf("count:%d\n", ++count);
        if (count == 1000000) {
            exit(0);
        }
        delete request_;
        delete response_;
        delete controller_;
    }

private:
    maid::Controller* controller_;
    EchoRequest* request_;
    EchoResponse* response_;
};


int main()
{
    Channel* channel = new Channel();
    channel->Connect("127.0.0.1", 5555, true);
    for(int i = 0; i < 1000000; ++i){
        channel->Update();
        Controller* controller = new Controller();
        EchoRequest* request = new EchoRequest();
        EchoResponse* response = new EchoResponse();
        Closure* closure = new Closure(controller, request, response);
        EchoService_Stub* stub = new EchoService_Stub(channel);
        request->set_message("hello");
        stub->Echo(controller, request, response, closure);
    }

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
