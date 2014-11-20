#ifndef _MAID_CONTROLLER_H_
#define _MAID_CONTROLLER_H_
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/common.h>

namespace maid
{

namespace proto
{
class ControllerMeta;
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

    void set_fd(int64_t fd);

    int64_t fd();

    void set_notify(bool notify);

    maid::proto::ControllerMeta& meta_data();

    virtual ~Controller();

private:
    ControllerImpl* controller_;
};

} /* maid */

#endif /*_MAID_CONTROLLER_H_*/
