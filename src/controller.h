#pragma once

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/common.h>

namespace maid
{

namespace proto
{
    class ControllerProto;
}

class Controller : public google::protobuf::RpcController
{
public:
    Controller();
    virtual ~Controller();

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

public: // unit test only
    inline const google::protobuf::Closure* cancel_callback() const
    {
        return cancel_callback_;
    }

private:
    proto::ControllerProto* proto_;
    google::protobuf::Closure* cancel_callback_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Controller);
};

} /* maid */
