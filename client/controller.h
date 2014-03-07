#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/common.h>
#include "controller.pb.h"

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

    google::protobuf::Message& get_meta_data() const;

private:
    google::protobuf::Message meta_data_;
};

#endif /*CONTROLLER_H*/
