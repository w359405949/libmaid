#pragma once
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/common.h>

namespace maid
{

namespace proto
{
class ControllerProto;
}

class ControllerImpl;

class Controller : public google::protobuf::RpcController
{
public:
    Controller();

    void Reset();

    bool Failed() const;

    std::string ErrorText() const;

    void StartCancel();

    void SetFailed(const std::string& reason);

    bool IsCanceled() const;

    void NotifyOnCancel(google::protobuf::Closure* callback);

    proto::ControllerProto* mutable_proto();
    const proto::ControllerProto& proto();

    int64_t connection_id();
    void set_connection_id(int64_t connection_id);

    virtual ~Controller();

private:
    ControllerImpl* controller_;
};

} /* maid */
