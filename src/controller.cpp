#include <stdio.h>
#include "controller.h"

using maid::controller::Controller;
using maid::proto::ControllerMeta;

Controller::Controller(struct ev_loop* loop)
    :request_(NULL),
    response_(NULL),
    done_(NULL),
    next_(NULL),

    loop_(loop),
    ref_(0),
    in_gc_(false)
{
    gc_.data = this;
    meta_data_.set_failed(false);
}

Controller::~Controller()
{
    assert(0 >= ref_ && "normal delete");
    if(!meta_data_.stub() && NULL != request_){
        delete request_;
    }
    if(!meta_data_.stub() && NULL != response_){
        delete response_;
    }
    done_ = NULL;
    next_ = NULL;
    loop_ = NULL;
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

ControllerMeta& Controller::meta_data()
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

google::protobuf::Message* Controller::request()
{
    return request_;
}

google::protobuf::Message* Controller::response()
{
    return response_;
}

google::protobuf::Closure* Controller::done()
{
    return done_;
}

Controller* Controller::next()
{
    return next_;
}

void Controller::set_fd(int32_t fd)
{
    fd_ = fd;
}

int32_t Controller::fd()
{
    return fd_;
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
    if(1 <= ref_ && in_gc_){
        ev_check_stop(loop_, &gc_);
        in_gc_ = false;
    }

}

void Controller::OnGC(EV_P_ ev_check* w, int32_t revents)
{
    Controller* self = (Controller*)w->data;
    ev_check_stop(EV_A_ w);
    delete self;
}
