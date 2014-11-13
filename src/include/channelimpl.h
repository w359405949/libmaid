#ifndef _MAID_CHANNELIMPL_H_
#define _MAID_CHANNELIMPL_H_
#include <map>
#include <set>
#include <stack>

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <uv.h>
#include "buffer.h"
//#include "context.h"

namespace maid
{

class Controller;
class RemoteClosure;
class Context;

namespace proto
{
class ControllerMeta;
}

class ChannelImpl : public google::protobuf::RpcChannel
{
public:
    ChannelImpl(uv_loop_t* loop=NULL);
    virtual ~ChannelImpl();

    virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
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
    inline void set_default_fd(int64_t fd)
    {
        default_fd_ = fd;
    }

    int64_t default_fd()
    {
        return default_fd_;
    }

    inline void Update()
    {
        uv_run(loop_, UV_RUN_NOWAIT);
    }

    inline void ServeForever()
    {
        uv_run(loop_, UV_RUN_DEFAULT);
    }

    inline uv_loop_t* loop()
    {
        return loop_;
    }

public:
    virtual void SendRequest(Controller* controller, const google::protobuf::Message* request, google::protobuf::Message* response, google::protobuf::Closure* done);
    virtual void SendResponse(Controller* controller, const google::protobuf::Message* response);
    virtual void SendNotify(Controller* controller, const google::protobuf::Message* request);

    virtual int32_t Handle(uv_stream_t* handle, ssize_t nread);
    virtual int32_t HandleRequest(Controller* controller);
    virtual int32_t HandleResponse(Controller* controller);
    virtual int32_t HandleNotify(Controller* controller);

    virtual void AddConnection(uv_stream_t* handle);
    virtual void RemoveConnection(uv_stream_t* handle);

    virtual RemoteClosure* NewRemoteClosure();
    virtual void DeleteRemoteClosure(RemoteClosure* done);

    static inline int64_t StreamToFd(uv_stream_t* stream)
    {
        return (int64_t)stream;
    }
    static inline uv_stream_t* FdToStream(int64_t fd)
    {
        return (uv_stream_t*)fd;
    }

public: /* unit test only */
    static void OnRead(uv_stream_t* w, ssize_t nread, const uv_buf_t* buf);
    static void OnAccept(uv_stream_t* handle, int32_t status);
    static void OnConnect(uv_connect_t* handle, int32_t status);
    static void OnAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
    static void AfterSendRequest(uv_write_t* req, int32_t status);
    static void AfterSendResponse(uv_write_t* req, int32_t status);
    static void AfterSendNotify(uv_write_t* req, int32_t status);
    static void OnClose(uv_handle_t* handle);
    static void OnRemoteClosureGC(uv_idle_t* handle);

public: /* unit test only */
    std::map<std::string, google::protobuf::Service*> service_; //<full_service_name, service>
    std::map<int64_t, Context> async_result_; //<transmit_id, Context>
    std::map<int64_t, uv_stream_t*> connected_handle_; //<fd, stream>
    std::map<int64_t, uv_stream_t*> listen_handle_; //<fd, stream>
    std::map<int64_t, Buffer> buffer_;//<fd, buffer>
    std::map<int64_t, std::set<int64_t> > transactions_; //<fd, <transmit_id> >
    std::stack<RemoteClosure*> remote_closure_pool_;

public:  /* unit test only */
    // libuv
    uv_loop_t* loop_;
    uv_idle_t remote_closure_gc_;

    // packet
    const uint32_t controller_max_length_;

    //
    int64_t default_fd_; //uv_stream_t
};

} /* namespace maid */

#endif /*_MAID_INTERNAL_CHANNEL_H_*/
