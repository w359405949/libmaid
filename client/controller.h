#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/common.h>

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
};

#endif /*CONTROLLER_H*/
