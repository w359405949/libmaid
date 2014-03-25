#include <stdio.h>
#include "channel.h"
#include "closure.h"
#include "controller.h"

using maid::channel::Channel;
using maid::closure::SmartClosure;
using maid::closure::RemoteClosure;
using maid::controller::Controller;

SmartClosure::SmartClosure(struct ev_loop* loop, Channel* channel, Controller* controller)
    :loop_(loop),
    channel_(channel),
    controller_(controller)
{
    assert(("channel can not be NULL", NULL != channel));
    assert(("loop can not be NULL", NULL != loop));
    assert(("controller can not be NULL", NULL != controller));
    gc_.data = this;
}

SmartClosure::~SmartClosure()
{
    loop_ = NULL;
    channel_ = NULL;
    controller_ = NULL;
}

void SmartClosure::Run()
{
    DoRun();
    ev_check_init(&gc_, OnGC);
    ev_check_start(loop_, &gc_);
}

void SmartClosure::DoRun()
{
}

void SmartClosure::OnGC(EV_P_ ev_check* w, int32_t revents)
{
    SmartClosure* self = (SmartClosure*)(w->data);
    ev_check_stop(EV_A_ w);
    delete self;
}

maid::channel::Channel* SmartClosure::get_channel()
{
    return channel_;
}

maid::controller::Controller* SmartClosure::get_controller()
{
    return controller_;
}

RemoteClosure::RemoteClosure(struct ev_loop* loop, Channel* channel, Controller* controller)
    :SmartClosure(loop, channel, controller)
{
}

void RemoteClosure::DoRun()
{
    get_channel()->PushController(get_controller()->fd(), get_controller());
    /*
     * TODO: check PushController return value. and retry if needed.
     */
}
