#include <google/protobuf/message.h>

#include "remoteclosure.h"
#include "channelimpl.h"
#include "define.h"
#include "maid/controller.h"

using maid::RemoteClosure;

RemoteClosure::RemoteClosure(ChannelImpl* channel)
    :channel_(channel),
    controller_(NULL),
    response_(NULL),
    request_(NULL)
{
}

RemoteClosure::~RemoteClosure()
{
    Reset();
}

void RemoteClosure::Reset()
{
    delete controller_;
    delete request_;
    delete response_;

    controller_ = NULL;
    response_ = NULL;
    request_ = NULL;

}

void RemoteClosure::Run()
{
    if (controller_ != NULL ) {
        channel_->SendResponse(controller_->mutable_proto(), response_);
    }
    channel_->DeleteRemoteClosure(this);
}
