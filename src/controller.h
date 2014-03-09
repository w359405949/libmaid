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

    maid::proto::Controller& get_meta_data();

    void Ref();
    void Unref();
    int32_t get_ref();

    void FailedRef();
    int32_t get_failed_ref();

private:
    maid::proto::Controller meta_data_;
    int32_t ref_;
    int32_t failed_ref_;
};

} /* controller */

} /* maid */

#endif /*CONTROLLER_H*/
