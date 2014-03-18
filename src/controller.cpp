#include <stdio.h>
#include "controller.h"

using maid::controller::Controller;
using maid::proto::ControllerMeta;

Controller::Controller(struct ev_loop* loop)
    :loop_(loop),
    in_gc_(false)
{
    gc_.data = this;
}

Controller::~Controller()
{
    assert(("normal delete", 0 >= ref_));
}

void Controller::Reset()
{
}

bool Controller::Failed() const
{
    return meta_data_.failed();
}

std::string Controller::ErrorText() const
{
    return meta_data_.error_text();
}

void Controller::StartCancel()
{
}

void Controller::SetFailed(const std::string& reason)
{
    meta_data_.set_failed(true);
    meta_data_.set_error_text(reason);
}

bool Controller::IsCanceled() const
{
    return meta_data_.is_canceled();
}

void Controller::NotifyOnCancel(google::protobuf::Closure* callback)
{
}

ControllerMeta& Controller::get_meta_data()
{
    return meta_data_;
}

void Controller::set_request(google::protobuf::Message* request)
{
    request_ = request;
}

void Controller::set_response(google::protobuf::Message* response)
{
    response_ = response;
}

void Controller::set_done(google::protobuf::Closure* done)
{
    done_ = done;
}

void Controller::set_next(Controller* next)
{
    next_ = next;
}

google::protobuf::Message* Controller::get_request()
{
    return request_;
}

google::protobuf::Message* Controller::get_response()
{
    return response_;
}

google::protobuf::Closure* Controller::get_done()
{
    return done_;
}

Controller* Controller::get_next()
{
    return next_;
}

void Controller::Destroy()
{
    --ref_;
    if(0 >= ref_ && !in_gc_){
        ev_check_init(&gc_, OnGC);
        ev_check_start(loop_, &gc_);
        in_gc_ = true;
    }
}

void Controller::Ref()
{
    ++ref_;
    if(1 >= ref_ && in_gc_){
        ev_check_stop(loop_, &gc_);
        in_gc_ = false;
    }

}

void Controller::OnGC(EV_P_ ev_check* w, int32_t revents)
{
    Controller* self = (Controller*)w->data;
    printf("libmaid: GC, %d, %d\n", self->meta_data_.fd(), self->meta_data_.transmit_id());
    ev_check_stop(EV_A_ w);
    delete self;
}
