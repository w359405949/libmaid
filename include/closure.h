#ifndef MAID_CLOSURE_REMOTECLOSURE_H
#define MAID_CLOSURE_REMOTECLOSURE_H
#include <google/protobuf/service.h>
#include <ev.h>

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

class SmartClosure: public google::protobuf::Closure
{
public:
    SmartClosure(struct ev_loop* loop, maid::channel::Channel* channel,
            maid::controller::Controller* controller);
    void Run();

protected:
    virtual void DoRun();
    maid::channel::Channel* channel();
    maid::controller::Controller* controller();
    virtual ~SmartClosure();

private:
    static void OnGC(EV_P_ ev_check* w, int32_t revents);

    SmartClosure& operator=(SmartClosure& other); // disable evil constructor
    SmartClosure(SmartClosure& other); // disable evil constructor

private:
    maid::channel::Channel* channel_;
    maid::controller::Controller* controller_;

    struct ev_check gc_;
    struct ev_loop* loop_;
    int32_t count_;
};



class RemoteClosure : public SmartClosure
{
public:
    RemoteClosure(struct ev_loop* loop, maid::channel::Channel* channel,
            maid::controller::Controller* controller);
protected:
    void DoRun();
};

} /* closure */

} /* maid */
#endif /* MAID_CLOSURE_REMOTECLOSURE_H */
