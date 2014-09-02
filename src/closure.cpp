#include <stdio.h>
#include "channel.h"
#include "closure.h"
#include "controller.h"

using maid::channel::Channel;
using maid::closure::SmartClosure;
using maid::closure::RemoteClosure;
using maid::controller::Controller;

SmartClosure::SmartClosure(struct ev_loop* loop, Channel* channel, Controller* controller)
    :channel_(channel),
    controller_(controller),
    loop_(loop),
    count_(0)
{
    assert(NULL != channel && "channel can not be NULL");
    assert(NULL != loop && "loop can not be NULL");
    assert(NULL != controller && "controller can not be NULL");
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
    ++count_;
    assert(1 == count_ && "done->Run() can not be called twice.");
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

maid::channel::Channel* SmartClosure::channel()
{
    return channel_;
}

maid::controller::Controller* SmartClosure::controller()
{
    return controller_;
}

struct ev_loop* SmartClosure::loop()
{
    return loop_;
}

RemoteClosure::RemoteClosure(struct ev_loop* loop, Channel* channel, Controller* controller)
    :SmartClosure(loop, channel, controller)
{
}

void RemoteClosure::DoRun()
{
    channel()->PushController(controller()->fd(), controller());
    /*
     * TODO: check PushController return value. and retry if needed.
     */
}
