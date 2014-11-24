#ifndef _MAID_CLOSUREIMPL_H_
#define _MAID_CLOSUREIMPL_H_
#include <google/protobuf/service.h>

namespace maid
{
class ChannelImpl;
class Controller;

class RemoteClosure : public google::protobuf::Closure
{
public:
    RemoteClosure(ChannelImpl* channel);
    ~RemoteClosure();

    inline void set_response(google::protobuf::Message* response)
    {
        response_ = response;
    }

    inline void set_controller(Controller* controller)
    {
        controller_ = controller;
    }

    inline void set_request(google::protobuf::Message* request)
    {
        request_ = request;
    }

    inline google::protobuf::Message* response()
    {
        return response_;
    }

    inline google::protobuf::Message* request()
    {
        return request_;
    }

    inline Controller* controller()
    {
        return controller_;
    }

    inline ChannelImpl* channel()
    {
        return channel_;
    }

    virtual void Run();

private:
    ChannelImpl* channel_;
    Controller* controller_;
    google::protobuf::Message* response_;
    google::protobuf::Message* request_;
};

} /* maid */
#endif /* _MAID_INTERNAL_CLOSURE_H_*/
