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

void RemoteClosure::Run()
{
    channel_->SendResponse(controller_, response_);

    delete request_;
    delete controller_;
    delete response_;
    controller_ = NULL;
    response_ = NULL;
    request_ = NULL;

    channel_->DeleteRemoteClosure(this);
}
