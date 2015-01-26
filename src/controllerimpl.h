#pragma once

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/common.h>

#include "maid/controller.pb.h"

namespace maid
{

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
    inline proto::ControllerProto* mutable_proto()
    {
        return &proto_;
    }
    inline const proto::ControllerProto& proto()
    {
        return proto_;
    }

    void set_connection_id(int64_t connection_id);
    int64_t connection_id() const;

public:
    proto::ControllerProto proto_;
};

} /* maid */
