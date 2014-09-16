#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "maid.h"
#include "echo.pb.h"

using maid::channel::Channel;
using maid::controller::Controller;

int count = 0;
int success_count = 0;

class Closure : public maid::closure::Closure
{
public:
    void Run()
    {
        //printf("==========\n%s\n-------\n%s\n", controller()->meta_data().DebugString().c_str(), response()->DebugString().c_str());
        if (!controller()->Failed()) {
            //printf("success count:%d\n", ++success_count);
            count++;
        }
        //printf("count:%d\n", ++count);
        if (count == 1000000) {
            exit(0);
        }
    }

};


int main()
{
    Channel* channel = new Channel(uv_default_loop());
    channel->Connect("127.0.0.1", 8888, true);
    for(int i = 0; i < 1000000; ++i){
        uv_run(uv_default_loop(), UV_RUN_NOWAIT);
        Controller* controller = new Controller();
        EchoRequest* request = new EchoRequest();
        EchoResponse* response = new EchoResponse();
        Closure* closure = new Closure();
        EchoService_Stub* stub = new EchoService_Stub(channel);
        request->set_message("hello");
        stub->Echo(controller, request, response, closure);
    }

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
