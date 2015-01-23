#pragma once

#include <stdint.h>
#include <google/protobuf/service.h>
#include <uv.h>

namespace maid
{
namespace proto
{
class Middleware;
}

class ChannelImpl;

class Channel : public google::protobuf::RpcChannel
{
public:
    Channel(uv_loop_t* loop=NULL);
    virtual ~Channel();

    void CallMethod(const google::protobuf::MethodDescriptor* method,
            google::protobuf::RpcController* controller,
            const google::protobuf::Message* request,
            google::protobuf::Message* response,
            google::protobuf::Closure* done);

    /*
     * add a Listen/Connect Address
     * return
     * < 0: error happend
     * > 0: fd.
     */
    int64_t Listen(const char* host, int32_t port, int32_t backlog=1);

    int64_t Connect(const char* host, int32_t port, bool as_default=false);

    /*
     * service for remote request
     */
    void AppendService(google::protobuf::Service* service);


    /*
     *
     */
    void AppendMiddleware(maid::proto::Middleware* middleware);

    /*
     * *HOT* method
     */
    void set_default_connection_id(int64_t fd);

    int64_t default_connection_id();

    void Update();

    void ServeForever();

    uv_loop_t* loop();

private:
    ChannelImpl* channel_;
};

} /* namespace maid */
