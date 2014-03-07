#include "controller.h"

Controller::Controller()
{
}

void Controller::Reset()
{
}

bool Controller::Failed() const
{
    return false;
}

std::string Controller::ErrorText() const
{
    return "";
}

void Controller::StartCancel()
{
}

void Controller::SetFailed(const std::string& reason)
{
}

bool Controller::IsCanceled() const
{
    return false;
}

void Controller::NotifyOnCancel(google::protobuf::Closure* callback)
{
}
