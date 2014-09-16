#ifndef MAID_CHANNEL_H
#define MAID_CHANNEL_H
#include <map>
#include <set>
#include <stack>

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <uv.h>

namespace maid
{

namespace proto
{
class ControllerMeta;
}

namespace controller
{
class Controller;
}

namespace closure
{
class Closure;
class RemoteClosure;
}

namespace channel
{

struct Buffer
{
    void* base;
    size_t len;
    size_t total;


    Buffer()
        :base(NULL),
        len(0),
        total(0)
    {
    }

    ~Buffer()
    {
        free(base);
    }

    void Expend(size_t expect_length);
};

class Channel : public google::protobuf::RpcChannel
{
public:
    Channel(uv_loop_t* loop);
    virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done);

    virtual ~Channel();

    /*
     * add a Listen/Connect Address
     * return
     * < 0: error happend
     * > 0: fd.
     */
    int64_t Listen(const std::string& host, int32_t port, int32_t backlog=1);
    int64_t Connect(const std::string& host, int32_t port, bool as_default=false);

    /*
     * service for remote request
     */
    void AppendService(google::protobuf::Service* service);

    void SendRequest(maid::controller::Controller* controller, const google::protobuf::Message* request, maid::closure::Closure* done);
    void SendResponse(maid::controller::Controller* controller, const google::protobuf::Message* response);
    void SendNotify(maid::controller::Controller* controller, const google::protobuf::Message* request);

    inline void set_default_fd(int64_t fd)
    {
        default_fd_ = fd;
    }

    int64_t default_fd()
    {
        return default_fd_;
    }

    maid::closure::RemoteClosure* NewRemoteClosure();
    void DestroyRemoteClosure(maid::closure::RemoteClosure* done);

private:
    static void OnRead(uv_stream_t* w, ssize_t nread, const uv_buf_t* buf);
    static void OnAccept(uv_stream_t* handle, int32_t status);
    static void OnConnect(uv_connect_t* handle, int32_t status);
    static void OnAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
    static void AfterSendRequest(uv_write_t* req, int32_t status);
    static void AfterSendResponse(uv_write_t* req, int32_t status);
    static void AfterSendNotify(uv_write_t* req, int32_t status);
    static void OnClose(uv_handle_t* handle);
    static void OnRemoteClosureGC(uv_idle_t* handle);

private:
    void Handle(uv_stream_t* handle, ssize_t nread);
    int32_t HandleRequest(maid::controller::Controller* controller);
    int32_t HandleResponse(maid::controller::Controller* controller);
    int32_t HandleNotify(maid::controller::Controller* controller);

    void NewConnection(uv_stream_t* handle);
    void CloseConnection(uv_stream_t* handle);

private:
    std::map<std::string, google::protobuf::Service*> service_; //<service_name, service>
    std::map<int64_t, maid::closure::Closure*> async_result_; //<transmit_id, controller>
    std::map<int64_t, uv_stream_t*> connected_handle_; //<fd, stream>
    std::map<int64_t, uv_stream_t*> listen_handle_; //<fd, stream>
    std::map<int64_t, Buffer> buffer_;//<fd, buffer>
    std::map<int64_t, std::set<int64_t> > transactions_; //<fd, <transmit_id> >
    std::stack<maid::closure::RemoteClosure*> closure_pool_;

private:
    // libuv
    uv_loop_t* loop_;
    uv_idle_t remote_closure_gc_;

    // packet
    const size_t controller_max_length_;

    //
    int64_t default_fd_; //uv_stream_t
};

} /* namespace channel */

} /* namespace maid */

#endif /*MAID_CHANNEL_H*/
