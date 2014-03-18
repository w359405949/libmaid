#include <stdio.h>
#include "channel.h"
#include "closure.h"
#include "controller.h"

using maid::channel::Channel;
using maid::closure::RemoteClosure;
using maid::controller::Controller;

RemoteClosure::RemoteClosure(struct ev_loop* loop, Channel* channel, Controller* controller)
    :loop_(loop),
    channel_(channel),
    controller_(controller)
{
    assert(("channel can not be NULL", NULL != channel));
    assert(("loop can not be NULL", NULL != loop));
    assert(("controller can not be NULL", NULL != controller));
    gc_.data = this;
}

RemoteClosure::~RemoteClosure()
{
    loop_ = NULL;
    channel_ = NULL;
    controller_ = NULL;
}

void RemoteClosure::Run()
{
    channel_->PushController(controller_->get_meta_data().fd(), controller_);
    ev_check_init(&gc_, OnGC);
    ev_check_start(loop_, &gc_);
    /*
     * TODO: check PushController return value. and retry if needed.
     */
}

void RemoteClosure::OnGC(EV_P_ ev_check* w, int32_t revents)
{
    RemoteClosure* self = (RemoteClosure*)(w->data);
    ev_check_stop(EV_A_ w);
    delete self;
}
