#pragma once
#include <google/protobuf/service.h>
#include <google/protobuf/map.h>
#include <google/protobuf/repeated_field.h>
#include <uv.h>

#include "context.h"
#include "buffer.h"

namespace maid
{

namespace proto
{
class ControllerProto;
class ConnectionProto;
}

class AbstractTcpChannelFactory;
class Closure;
class Controller;


class Channel : public google::protobuf::RpcChannel
{
public:
    Channel();
    virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done) override;

public:
    static inline Channel* default_instance()
    {
        return default_instance_;
    }

    friend void InitChannel();

private:
    static Channel* default_instance_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Channel);
};


class TcpChannel : public google::protobuf::RpcChannel
{
public:
    TcpChannel(uv_loop_t* loop, uv_stream_t* stream, AbstractTcpChannelFactory* factory);
    virtual ~TcpChannel();

    virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done);

public:
    virtual int32_t Handle();
    virtual int32_t HandleRequest(proto::ControllerProto* controller_proto);
    virtual int32_t HandleResponse(proto::ControllerProto* controller_proto);

    void RemoveController(Controller* controller);
    void Close();

    inline uv_stream_t* stream() const
    {
        return stream_;
    }

    AbstractTcpChannelFactory* factory();

public:
    static void AfterSendRequest(uv_write_t* req, int32_t status);
    static void OnRead(uv_stream_t* w, ssize_t nread, const uv_buf_t* buf);
    static void OnAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
    static void OnIdle(uv_idle_t* handle);
    static void OnTimer(uv_timer_t* timer);
    static void OnCloseStream(uv_handle_t* handle);

public: // unit test only
    inline const google::protobuf::Map<int64_t, Context>& async_result() const
    {
        return async_result_;
    }

    inline const google::protobuf::Map<uv_write_t*, std::string*>& sending_buffer() const
    {
        return sending_buffer_;
    }

    inline const google::protobuf::Map<Controller*, Controller*>& router_controllers() const
    {
        return router_controllers_;
    }

    inline const uv_timer_t& timer_handle() const
    {
        return timer_handle_;
    }

    inline const uv_idle_t& idle_handle() const
    {
        return idle_handle_;
    }

    inline const Buffer& buffer() const
    {
        return buffer_;
    }

    inline const AbstractTcpChannelFactory* factory() const
    {
        return factory_;
    }

    inline int64_t transmit_id() const
    {
        return transmit_id_;
    }

private:
    google::protobuf::Map<int64_t/* transmit_id */, Context> async_result_;
    google::protobuf::Map<uv_write_t*, std::string* /* send_buffer */> sending_buffer_; //
    google::protobuf::Map<Controller*, Controller*> router_controllers_;

private:
    uv_loop_t* loop_;
    uv_stream_t* stream_;
    uv_timer_t timer_handle_;
    uv_idle_t idle_handle_;
    Buffer buffer_;

    AbstractTcpChannelFactory* factory_;

    // packet
    int64_t transmit_id_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(TcpChannel);
};

class LocalMapRepoChannel : public google::protobuf::RpcChannel
{
public:
    virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done);

    void Insert(google::protobuf::Service* service);
    void Close();

    LocalMapRepoChannel();
    ~LocalMapRepoChannel();

public: // unit test only
    inline const google::protobuf::Map<const google::protobuf::ServiceDescriptor*, google::protobuf::Service*> service() const
    {
        return service_;
    }

private:
    google::protobuf::Map<const google::protobuf::ServiceDescriptor*, google::protobuf::Service*> service_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(LocalMapRepoChannel);
};


class LocalListRepoChannel : public google::protobuf::RpcChannel
{
public:
    virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done);

    void Append(google::protobuf::Service* service);
    void Close();

    LocalListRepoChannel();
    ~LocalListRepoChannel();

public:
    inline const google::protobuf::RepeatedField<google::protobuf::Service*> service() const
    {
        return service_;
    }

private:
    google::protobuf::RepeatedField<google::protobuf::Service*> service_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(LocalListRepoChannel);
};
}
