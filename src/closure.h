#ifndef MAID_CHANNEL_H
#define MAID_CHANNEL_H
#include <google/protobuf/service.h>
#include <ev.h>

class maid::channel::Channel;
class maid::channel::Context;

namespace maid
{
namespace closure
{

class RemoteClosure : public google::protobuf::Closure
{
public:
    RemoteClosure(struct ev_loop* loop, maid::channel::Channel* channel,
            maid::channel::Context* context);
    ~RemoteClosure();
    void Run();

private:
    void OnGC(EV_P_ ev_check* w, int32_t revents);

private:
    maid::channel::Channel* channel_;
    maid::channel::Context* context_;

    struct ev_check gc_;
    struct ev_loop* loop_;
};

} /* closure */

} /* maid */
#endif /* MAID_CHANNEL_H */
