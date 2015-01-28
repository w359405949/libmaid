#include "controllerimpl.h"
#include "maid/connection.pb.h"

using maid::ControllerImpl;

ControllerImpl::ControllerImpl()
    :proto_(NULL)
{
}

ControllerImpl::~ControllerImpl()
{
    delete proto_;
}

void ControllerImpl::Reset()
{
}

bool ControllerImpl::Failed() const
{
    return proto_ == NULL ? proto::ControllerProto::default_instance().failed() : proto_->failed();
}

std::string ControllerImpl::ErrorText() const
{
    return proto_ == NULL ? proto::ControllerProto::default_instance().error_text() : proto_->error_text();
}

void ControllerImpl::StartCancel()
{
}

void ControllerImpl::SetFailed(const std::string& reason)
{
    mutable_proto();
    proto_->set_failed(true);
    proto_->set_error_text(reason);
}

bool ControllerImpl::IsCanceled() const
{
    return proto_ == NULL ? proto::ControllerProto::default_instance().is_canceled() : proto_->is_canceled();
}

void ControllerImpl::NotifyOnCancel(google::protobuf::Closure* callback)
{
}

void ControllerImpl::set_connection_id(int64_t connection_id)
{
    mutable_proto();

    proto::ConnectionProto* connection = proto_->MutableExtension(proto::connection);
    connection->set_id(connection_id);
}

int64_t ControllerImpl::connection_id() const
{
    const proto::ConnectionProto& connection = proto_ == NULL? proto::ConnectionProto::default_instance() : proto_->GetExtension(proto::connection);
    return connection.id();
}

maid::proto::ControllerProto* ControllerImpl::mutable_proto()
{
    if (proto_ == NULL) {
        proto_ = new proto::ControllerProto();
    }
    return proto_;
}

const maid::proto::ControllerProto& ControllerImpl::proto() const
{
    return proto_ == NULL ? proto::ControllerProto::default_instance() : *proto_;
}

maid::proto::ControllerProto* ControllerImpl::release_proto()
{
    proto::ControllerProto* proto = proto_;
    proto_ = NULL;
    return proto;
}

void ControllerImpl::set_allocated_proto(maid::proto::ControllerProto* proto)
{
    if (proto == proto_) {
        return;
    }
    delete proto_;
    proto_ = proto;
}
