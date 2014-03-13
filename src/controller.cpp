#include "controller.h"

using maid::controller::Controller;
using maid::proto::ControllerMeta;

Controller::Controller()
    :ref_(0)
{
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

void Controller::Ref()
{
    ++ref_;
}

void Controller::Unref()
{
    --ref_;
}

int32_t Controller::get_ref()
{
    return ref_;
}
