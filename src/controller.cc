#include <google/protobuf/stubs/logging.h>
#include "controller.h"
#include "maid/controller.pb.h"

using maid::Controller;

Controller::Controller()
    :proto_(NULL),
    cancel_callback_(NULL)
{
}

Controller::~Controller()
{
    delete cancel_callback_;
    delete proto_;
}

void Controller::Reset()
{
    if (NULL != proto_) {
        proto_->Clear();
    }
}

bool Controller::Failed() const
{
    return proto_ == NULL ? proto::ControllerProto::default_instance().failed() : proto_->failed();
}

std::string Controller::ErrorText() const
{
    return proto_ == NULL ? proto::ControllerProto::default_instance().error_text() : proto_->error_text();
}

void Controller::StartCancel()
{
    mutable_proto();
    proto_->set_is_canceled(true);

    if (cancel_callback_ != NULL) {
        cancel_callback_->Run();
    }
}

void Controller::SetFailed(const std::string& reason)
{
    mutable_proto();
    proto_->set_failed(true);
    proto_->set_error_text(reason);
}

bool Controller::IsCanceled() const
{
    return proto_ == NULL ? proto::ControllerProto::default_instance().is_canceled() : proto_->is_canceled();
}

void Controller::NotifyOnCancel(google::protobuf::Closure* callback)
{
    GOOGLE_CHECK(cancel_callback_ == NULL);
    cancel_callback_ = callback;
}

maid::proto::ControllerProto* Controller::mutable_proto()
{
    if (proto_ == NULL) {
        proto_ = new proto::ControllerProto();
    }
    return proto_;
}

const maid::proto::ControllerProto& Controller::proto() const
{
    return proto_ == NULL ? proto::ControllerProto::default_instance() : *proto_;
}

maid::proto::ControllerProto* Controller::release_proto()
{
    proto::ControllerProto* proto = proto_;
    proto_ = NULL;
    return proto;
}

void Controller::set_allocated_proto(maid::proto::ControllerProto* proto)
{
    if (proto == proto_) {
        return;
    }
    delete proto_;
    proto_ = proto;
}
