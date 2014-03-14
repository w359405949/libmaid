#ifndef MAID_CLOSURE_H
#define MAID_CLOSURE_H
#include <google/protobuf/service.h>
#include <ev.h>
namespace maid
{
namespace channel
{

class Channel;
class Context;

}

namespace closure
{

class RemoteClosure : public google::protobuf::Closure
{
public:
    RemoteClosure(struct ev_loop* loop_, maid::channel::Channel* channel,
            maid::channel::Context* context);
    ~RemoteClosure();
    void Run();

private:
    static void OnGC(EV_P_ ev_check* w, int32_t revents);

private:
    const maid::channel::Channel* channel_;
    const maid::channel::Context* context_;

    struct ev_check gc_;
    struct ev_loop* loop_;
};

} /* closure */

} /* maid */
#endif /* MAID_CLOSURE_H */
