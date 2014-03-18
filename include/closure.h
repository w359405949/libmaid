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

class RemoteClosure : public google::protobuf::Closure
{
public:
    RemoteClosure(struct ev_loop* loop_, maid::channel::Channel* channel,
            maid::controller::Controller* controller);
    ~RemoteClosure();
    void Run();

private:
    static void OnGC(EV_P_ ev_check* w, int32_t revents);

private:
    maid::channel::Channel* channel_;
    maid::controller::Controller* controller_;

    struct ev_check gc_;
    struct ev_loop* loop_;
};

} /* closure */

} /* maid */
#endif /* MAID_CLOSURE_REMOTECLOSURE_H */
