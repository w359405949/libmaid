#ifndef MAID_CONTROLLER_H
#define MAID_CONTROLLER_H
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/common.h>

#include "controller.pb.h"

namespace maid
{

namespace controller
{

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

    maid::proto::ControllerMeta& meta_data();

public:
    inline void set_fd(int64_t fd)
    {
        fd_ = fd;
    }

    inline int64_t fd()
    {
        return fd_;
    }

    virtual ~Controller(); // diable delete except GC;

private:
    maid::proto::ControllerMeta meta_data_;

    int64_t fd_;
};

} /* controller */

} /* maid */

#endif /*MAID_CONTROLLER_H*/
