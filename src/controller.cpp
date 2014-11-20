#include "maid/controller.h"
#include "controllerimpl.h"

using maid::Controller;

Controller::Controller()
{
    controller_ = new ControllerImpl();
}

void Controller::Reset()
{
    controller_->Reset();
}

bool Controller::Failed() const
{
    return controller_->Failed();
}

std::string Controller::ErrorText() const
{
    return controller_->ErrorText();
}

void Controller::StartCancel()
{
    return controller_->StartCancel();
}

void Controller::SetFailed(const std::string& reason)
{
    return controller_->SetFailed(reason);
}

bool Controller::IsCanceled() const
{
    return controller_->IsCanceled();
}

void Controller::NotifyOnCancel(google::protobuf::Closure* callback)
{
    return controller_->NotifyOnCancel(callback);
}

void Controller::set_fd(int64_t fd)
{
    controller_->set_fd(fd);
}

int64_t Controller::fd()
{
    return controller_->fd();
}

void Controller::set_notify(bool notify)
{
    controller_->set_notify(notify);
}

Controller::~Controller()
{
    delete controller_;
}

maid::proto::ControllerMeta& Controller::meta_data()
{
    return controller_->meta_data();
}
