#ifndef _MAID_CHANNEL_H_
#define _MAID_CHANNEL_H_

#include <stdint.h>
#include <google/protobuf/service.h>
#include <uv.h>

namespace maid
{

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

    void set_default_fd(int64_t fd);

    int64_t default_fd();

    void Update();

    void ServeForever();

    uv_loop_t* loop();

private:
    ChannelImpl* channel_;
};

} /* namespace maid */

#endif /*_MAID_CHANNEL_H_*/
