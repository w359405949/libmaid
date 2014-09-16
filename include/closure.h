#ifndef MAID_CLOSURE_REMOTECLOSURE_H
#define MAID_CLOSURE_REMOTECLOSURE_H
#include <google/protobuf/service.h>

namespace maid
{

namespace channel
{
class Channel;
}

namespace controller
{
class Controller;
}

namespace closure
{

class Closure : public google::protobuf::Closure
{
public:
    Closure()
        :controller_(NULL),
        request_(NULL),
        response_(NULL)
    {
    }

    inline void set_controller(maid::controller::Controller* controller)
    {
        controller_ = controller;
    }

    inline void set_request(const google::protobuf::Message* request)
    {
        request_ = request;
    }

    inline void set_response(google::protobuf::Message* response)
    {
        response_ = response;
    }

    inline maid::controller::Controller* controller()
    {
        return controller_;
    }

    inline const google::protobuf::Message* request()
    {
        return request_;
    }

    inline google::protobuf::Message* response()
    {
        return response_;
    }

protected:
    maid::controller::Controller* controller_;
    const google::protobuf::Message* request_;
    google::protobuf::Message* response_;
};


class RemoteClosure : public Closure
{
public:
    RemoteClosure(maid::channel::Channel* channel)
        :channel_(channel)
    {
    }

    virtual void Run();

private:
    maid::channel::Channel* channel_;
};

} /* closure */

} /* maid */
#endif /* MAID_CLOSURE_REMOTECLOSURE_H */
