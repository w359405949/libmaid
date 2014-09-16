#include <stdio.h>
#include "channel.h"
#include "closure.h"
#include "controller.h"

using maid::channel::Channel;
using maid::controller::Controller;
using maid::closure::Closure;
using maid::closure::RemoteClosure;

void RemoteClosure::Run()
{
    channel_->SendResponse(controller_, response_);
    channel_->DestroyRemoteClosure(this);
    delete controller_;
    delete request_;
    delete response_;
    controller_ = NULL;
    request_ = NULL;
    response_ = NULL;
}
