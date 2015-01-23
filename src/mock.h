#pragma once

#include <google/protobuf/service.h>


namespace maid
{

void InitialMaidMock();

class MockController : public google::protobuf::RpcController
{
public:
    inline void Reset()
    {
    }

    inline bool Failed() const
    {
        return false;
    }

    inline std::string ErrorText() const
    {
        return "";
    }

    inline void StartCancel()
    {
    }

    inline void SetFailed(const std::string& reason)
    {
    }

    inline bool IsCanceled() const
    {
        return false;
    }

    inline void NotifyOnCancel(google::protobuf::Closure* callback)
    {
    }

    inline static MockController* default_instance()
    {
        return default_instance_;
    }

private:
    friend void InitialMaidMock();

    static MockController* default_instance_;
};

class MockClosure : public google::protobuf::Closure
{
public:
    inline void Run()
    {
    }

    inline static MockClosure* default_instance()
    {
        return default_instance_;
    }

private:
    friend void InitialMaidMock();

    static MockClosure* default_instance_;
};

} /* namespace maid */
