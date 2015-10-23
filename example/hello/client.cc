#include <stdio.h>
#include <google/protobuf/empty.pb.h>
#include "maid/base.h"
#include "maid/controller.h"
#include "maid/callbacks.h"
#include "hello.pb.h"

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
        //request->PrintDebugString();
        done->Run();
    }

private:
};

class Client : public maid::TcpClient
{
public:
    Client()
        :count_(0)
    {
        ctrl_c_.data = this;
        uv_signal_init(mutable_loop(), &ctrl_c_);
        uv_signal_start(&ctrl_c_, &OnCtrlC, SIGINT);

        update_.data = this;
        uv_timer_init(mutable_loop(), &update_);
        uv_timer_start(&update_, &OnTimer, 0, 1);
    }

    void Close() override
    {
        maid::TcpClient::Close();
        uv_signal_stop(&ctrl_c_);
        uv_timer_stop(&update_);
        closure_component_.Clear();
    }


    void Timer()
    {
        count_++;

        //GOOGLE_LOG(INFO)<<"count:"<<count_;

        int c = 500;
        //if (count_ < 5000) {
        if (true) {
            while (c--) {
                maid::Controller* controller = new maid::Controller();
                maid::example::HelloRequest* request = new maid::example::HelloRequest();
                request->set_message("hello");
                maid::example::HelloResponse* response = new maid::example::HelloResponse();

                maid::example::HelloService_Stub stub(channel());
                stub.Hello(controller, request, response, closure_component_.NewClosure(&Client::Callback, this, controller, request, response));
            }
        } else if (count_ < 10000) {
            while (c--) {
                maid::Controller* controller = new maid::Controller();
                maid::example::HelloRequest* request = new maid::example::HelloRequest();
                request->set_message("empty");
                google::protobuf::Empty* response = new google::protobuf::Empty();

                maid::example::HelloService_Stub stub(channel());
                stub.HelloNotify(controller, request, response, closure_component_.NewClosure(controller, request, response));
            }
        } else {
            //Close();
        }
    }

private:
    void Callback(google::protobuf::RpcController* controller, maid::example::HelloRequest* request, maid::example::HelloResponse* response)
    {
        response->PrintDebugString();
    }

private:
    static void OnTimer(uv_timer_t* handle)
    {
        Client* self = (Client*)handle->data;
        self->Timer();
    }

    static void OnCtrlC(uv_signal_t* handle, int statu)
    {
        Client* self = (Client*)handle->data;
        self->Close();
    }

private:
    uv_signal_t ctrl_c_;
    uv_timer_t update_;
    maid::ClosureComponent closure_component_;

    int count_;
};

int main()
{
    Client* client = new Client();
    client->InsertService(new HelloServiceImpl());
    client->Connect("127.0.0.1", 5555);
    client->ServeForever();
    delete client;
}
