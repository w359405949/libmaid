#include "channel.h"
#include "closure.h"
#include "controller.h"

using maid::channel::Channel;
using maid::channel::Context;
using maid::closure::RemoteClosure;
using maid::controller::Controller;

RemoteClosure::RemoteClosure(struct ev_loop* loop, Channel* channel, Context* context)
    :loop_(loop),
    channel_(channel),
    context_(context),
    in_gc_(false)
{
    assert(("channel can not be NULL", NULL != channel));
    assert(("loop can not be NULL", NULL != loop));
    assert(("context can not be NULL", NULL != context));
    gc_.data = this;
}

RemoteClosure::~RemoteClosure()
{
    loop_ = NULL;
    channel_ = NULL;
    context_ = NULL;
}

void RemoteClosure::Run()
{
    Controller* controller = context_->controller_;
    channel_->AppendContext(controller->get_meta_data().fd(), context_);
    /*
     * TODO: check AppendContext return value. and retry if needed.
     */
    if(in_gc_){
        return;
    }
    ev_check_init(&gc_, OnGC);
    ev_check_start(loop_, &gc_);
    in_gc_ = true;
}

void RemoteClosure::OnGC(EV_P_ ev_check* w, int32_t revents)
{
    RemoteClosure* self = (RemoteClosure*)(w->data);
    ev_check_stop(self->loop_, &(self->gc_));
    delete self;
}
