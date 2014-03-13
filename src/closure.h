#ifndef MAID_CHANNEL_H
#define MAID_CHANNEL_H
#include <google/protobuf/service.h>

class maid::channel::Channel;

namespace maid
{
namespace closure
{

class RemoteClosure : public google::protobuf::Closure
{
public:
    RemoteClosure(maid::channel::Channel* channel,
            maid::channel::Context* context);
    void Run();

private:
    maid::channel::Channel* channel_;
    maid::channel::Context* context_;
};

} /* closure */

} /* maid */
#endif /* MAID_CHANNEL_H */
