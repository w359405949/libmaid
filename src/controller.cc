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

Controller::~Controller()
{
    delete controller_;
}

maid::proto::ControllerProto* Controller::mutable_proto()
{
    return controller_->mutable_proto();
}

const maid::proto::ControllerProto& Controller::proto() const
{
    return controller_->proto();
}

maid::proto::ControllerProto* Controller::release_proto()
{
    return controller_->release_proto();
}

void Controller::set_allocated_proto(maid::proto::ControllerProto* proto)
{
    controller_->set_allocated_proto(proto);
}

int64_t Controller::connection_id()
{
    return controller_->connection_id();
}

void Controller::set_connection_id(int64_t connection_id)
{
    controller_->set_connection_id(connection_id);
}
