#pragma once

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/common.h>

namespace maid
{

namespace proto
{
    class ControllerProto;
}

class ControllerImpl : public google::protobuf::RpcController
{
public:
    ControllerImpl();
    virtual ~ControllerImpl();

    void Reset();

    bool Failed() const;

    std::string ErrorText() const;

    void StartCancel();

    void SetFailed(const std::string& reason);

    bool IsCanceled() const;

    void NotifyOnCancel(google::protobuf::Closure* callback);

public:
    proto::ControllerProto* mutable_proto();
    const proto::ControllerProto& proto() const;
    proto::ControllerProto* release_proto();
    void set_allocated_proto(proto::ControllerProto* proto);

    void set_connection_id(int64_t connection_id);
    int64_t connection_id() const;

public:
    proto::ControllerProto* proto_;
};

} /* maid */
