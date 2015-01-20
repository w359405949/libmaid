#ifndef _MAID_CHANNELIMPL_H_
#define _MAID_CHANNELIMPL_H_
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
class ConnectionEventService;
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
    void AppendConnectionEventService(maid::proto::ConnectionEventService* event_service);

    void set_default_connection_id(int64_t connection_id);
    int64_t default_connection_id();

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

    uv_stream_t* connected_handle(Controller* controller);

public:
    virtual void SendRequest(Controller* controller, const google::protobuf::Message* request, google::protobuf::Message* response, google::protobuf::Closure* done);
    virtual void SendResponse(Controller* controller, const google::protobuf::Message* response);
    virtual void SendNotify(Controller* controller, const google::protobuf::Message* request);

    virtual int32_t Handle(uv_stream_t* handle, Buffer& buffer);
    virtual int32_t HandleRequest(Controller* controller, google::protobuf::Service* service, const google::protobuf::MethodDescriptor* method);
    virtual int32_t HandleNotify(Controller* controller, google::protobuf::Service* service, const google::protobuf::MethodDescriptor* method);
    virtual int32_t HandleResponse(Controller* controller);

    virtual void AddConnection(uv_stream_t* handle);
    virtual void RemoveConnection(uv_stream_t* handle);

    virtual RemoteClosure* NewRemoteClosure();
    virtual void DeleteRemoteClosure(RemoteClosure* done);

public:
    static void OnRead(uv_stream_t* w, ssize_t nread, const uv_buf_t* buf);
    static void OnAccept(uv_stream_t* handle, int32_t status);
    static void OnConnect(uv_connect_t* handle, int32_t status);
    static void OnAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
    static void AfterSendRequest(uv_write_t* req, int32_t status);
    static void AfterSendResponse(uv_write_t* req, int32_t status);
    static void AfterSendNotify(uv_write_t* req, int32_t status);
    static void OnClose(uv_handle_t* handle);

public:
    std::map<std::string, google::protobuf::Service*> service_; //<full_service_name, service>
    std::vector<proto::ConnectionEventService*> connection_event_service_; //
    std::map<int64_t, Context> async_result_; //<transmit_id, Context>
    std::map<int64_t, uv_stream_t*> connected_handle_; //<connect_id, stream>
    std::map<int64_t, uv_stream_t*> listen_handle_; //<connect_id, stream>
    std::map<int64_t, Buffer> buffer_;//<connect_id, buffer>
    std::map<int64_t, std::set<int64_t> > transactions_; //<connect_id, <transmit_id> >
    std::map<uv_write_t*, std::string*> sending_buffer_; //
    std::stack<RemoteClosure*> remote_closure_pool_;

public:
    // libuv
    uv_loop_t* loop_;
    uv_idle_t remote_closure_gc_;
    uv_stream_t* default_handle_;

    // packet
    const int32_t controller_max_length_;

    //
    int64_t transmit_id_max_;
};

} /* namespace maid */

#endif /*_MAID_INTERNAL_CHANNEL_H_*/
