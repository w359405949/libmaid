#include "controllerimpl.h"
#include "connection.pb.h"

using maid::ControllerImpl;

ControllerImpl::ControllerImpl()
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
    return proto_.failed();
}

std::string ControllerImpl::ErrorText() const
{
    return proto_.error_text();
}

void ControllerImpl::StartCancel()
{
}

void ControllerImpl::SetFailed(const std::string& reason)
{
    proto_.set_failed(true);
    proto_.set_error_text(reason);
}

bool ControllerImpl::IsCanceled() const
{
    return proto_.is_canceled();
}

void ControllerImpl::NotifyOnCancel(google::protobuf::Closure* callback)
{
}

void ControllerImpl::set_connection_id(int64_t connection_id)
{
    proto::ConnectionProto* connection = proto_.MutableExtension(proto::connection);
    connection->set_id(connection_id);
}

int64_t ControllerImpl::connection_id()
{
    const proto::ConnectionProto connection = proto_.GetExtension(proto::connection);
    return connection.id();
}
