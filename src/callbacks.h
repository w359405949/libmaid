#pragma once

#include <functional>
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>

namespace maid {

template<typename FuncType, class Request, class Response>
class Closure : public google::protobuf::Closure
{
public:
    Closure(FuncType func, google::protobuf::RpcController* controller, Request* request, Response* response)
    :func_(func),
    controller_(controller),
    request_(request),
    response_(response)
    {
    }

    virtual void Run() override
    {
        func_(controller_, request_, response_);

        delete controller_;
        delete request_;
        delete response_;
        delete this;
    }

private:
    FuncType func_;
    google::protobuf::RpcController* controller_;
    Request* request_;
    Response* response_;
};

}
