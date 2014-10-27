#include "controllerimpl.h"

using maid::ControllerImpl;
using maid::proto::ControllerMeta;

ControllerImpl::ControllerImpl()
    :fd_(0)
{
}

ControllerImpl::~ControllerImpl()
{
}

void ControllerImpl::Reset()
{
}

bool ControllerImpl::Failed() const
{
    return meta_data_.failed();
}

std::string ControllerImpl::ErrorText() const
{
    return meta_data_.error_text();
}

void ControllerImpl::StartCancel()
{
}

void ControllerImpl::SetFailed(const std::string& reason)
{
    meta_data_.set_failed(true);
    meta_data_.set_error_text(reason);
}

bool ControllerImpl::IsCanceled() const
{
    return meta_data_.is_canceled();
}

void ControllerImpl::NotifyOnCancel(google::protobuf::Closure* callback)
{
}

ControllerMeta& ControllerImpl::meta_data()
{
    return meta_data_;
}
