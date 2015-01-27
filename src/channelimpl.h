#pragma once

#include <map>
#include <set>
#include <stack>

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <uv.h>
#include "buffer.h"
#include "context.h"

namespace maid
{

class Controller;
class RemoteClosure;
class Context;

namespace proto
{
class Middleware;
class ControllerProto;
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
    void AppendMiddleware(maid::proto::Middleware* middleware);

    void set_default_connection_id(int64_t connection_id);
    int64_t default_connection_id();

    inline uv_loop_t* loop()
    {
        return loop_;
    }

    uv_stream_t* connected_handle(Controller* controller);

    inline void Update()
    {
        uv_run(loop_, UV_RUN_NOWAIT);
    }

    inline void ServeForever()
    {
        uv_run(loop_, UV_RUN_DEFAULT);
    }

public:
    virtual void SendRequest(const google::protobuf::MethodDescriptor* method, Controller* controller);
    virtual void SendResponse(Controller* controller, const google::protobuf::Message* response);

    virtual int32_t Handle(int64_t connection_id);
    virtual int32_t HandleRequest(proto::ControllerProto* proto, int64_t connection_id);
    virtual int32_t HandleResponse(proto::ControllerProto* proto, int64_t connection_id);

    virtual void AddConnection(uv_stream_t* handle);
    virtual void RemoveConnection(uv_stream_t* handle);

    virtual RemoteClosure* NewRemoteClosure();
    virtual void DeleteRemoteClosure(RemoteClosure* done);

public:
    static void OnRead(uv_stream_t* w, ssize_t nread, const uv_buf_t* buf);
    static void OnAccept(uv_stream_t* handle, int32_t status);
    static void OnConnect(uv_connect_t* handle, int32_t status);
    static void OnAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
    static void OnCloseConnection(uv_handle_t* handle);
    static void OnCloseListen(uv_handle_t* handle);
    static void OnIdle(uv_idle_t* handle);
    static void OnTimer(uv_timer_t* timer);
    static void AfterSendRequest(uv_write_t* req, int32_t status);
    static void AfterSendResponse(uv_write_t* req, int32_t status);

public:
    std::map<std::string, google::protobuf::Service*> service_; //<full_service_name, service>
    std::vector<proto::Middleware*> middleware_; //
    std::map<int64_t/* connection_id */, uv_stream_t*> connected_handle_;
    std::map<int64_t/* connection_id */, uv_stream_t*> listen_handle_;
    std::map<int64_t/* connection_id */, uv_timer_t> timer_handle_; //
    std::map<int64_t/* connection_id */, uv_idle_t> idle_handle_; //
    std::map<int64_t/* connection_id */, Buffer> buffer_;
    std::map<int64_t/* connection_id */, std::set<int64_t> > transactions_; //<connect_id, <transmit_id> >
    std::map<int64_t/* transmit_id */, Context> async_result_; //<transmit_id, Context>
    std::map<uv_write_t*, int8_t* /* send_buffer */> sending_buffer_; //

    std::stack<RemoteClosure*> remote_closure_pool_;

public:
    // libuv
    uv_loop_t* loop_;
    uv_stream_t* default_stream_;

    // packet
    const int32_t controller_max_size_;

    //
    int64_t transmit_id_max_;
};

} /* namespace maid */
