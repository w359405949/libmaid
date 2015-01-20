#include <google/protobuf/service.h>

namespace maid
{

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

};

class MockClosure : public google::protobuf::Closure
{
public:
    inline void Run()
    {
    }
};

} /* namespace maid */
