#pragma once
#include <set>

#include "callbacks.h"

namespace maid {

class ClosureComponent
{
public:
    ClosureComponent()
    {

    }

    virtual ~ClosureComponent()
    {
        Clear();
    }

    inline void Clear()
    {
        for (auto controller : controllers_) {
            controller->StartCancel();
        }
        controllers_.clear();
    }

    // class-obj
    template<typename Func, class Class, class Request, class Response>
    google::protobuf::Closure*  NewClosure(Func _func, Class* _obj, google::protobuf::RpcController* _controller, Request* _request, Response* _response)
    {
        auto wrap_func = std::bind(_func, _obj, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        return NewClosure(wrap_func, _controller, _request, _response);
    }

    // std::function
    template<typename Func, class Request, class Response>
    google::protobuf::Closure*  NewClosure(Func _func, google::protobuf::RpcController* _controller, Request* _request, Response* _response)
    {
        auto wrap_func = [_func, this](google::protobuf::RpcController* controller, Request* request, Response* response){
            if (!controller->IsCanceled()) {
                GOOGLE_CHECK(controllers_.find(controller) != controllers_.end());
                controllers_.erase(controller);
                _func(controller, request, response);
            }
        };

        controllers_.insert(_controller);
        return new Closure<decltype(wrap_func), Request, Response>(wrap_func, _controller, _request, _response);
    }

    // mock
    template<class Request, class Response>
    google::protobuf::Closure*  NewClosure(google::protobuf::RpcController* _controller, Request* _request, Response* _response)
    {
        auto wrap_func = [](google::protobuf::RpcController* controller, Request* request, Response* response){};
        return NewClosure(wrap_func, _controller, _request, _response);
    }

private:
    std::set<google::protobuf::RpcController*> controllers_;
};

}
