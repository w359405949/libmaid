#ifndef _MAID_CONTROLLERIMPL_H_
#define _MAID_CONTROLLERIMPL_H_
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/common.h>

#include "controller.pb.h"

namespace maid
{
namespace proto
{
class ControllerMeta;
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

    proto::ControllerMeta& meta_data();

public:
    inline void set_fd(int64_t fd)
    {
        fd_ = fd;
    }

    inline int64_t fd()
    {
        return fd_;
    }

    inline void set_notify(bool notify)
    {
        meta_data_.set_notify(notify);
    }

public: /* unittest only */
    proto::ControllerMeta meta_data_;

    int64_t fd_;
};

} /* maid */

#endif /*_MAID_CONTROLLERIMPL_H_*/
