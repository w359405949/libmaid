#pragma once
#include <google/protobuf/service.h>
#include <google/protobuf/map.h>
#include <google/protobuf/repeated_field.h>
#include <uv.h>

#include "zero_copy_stream.h"
#include "component.h"

namespace maid
{

namespace proto
{
class ControllerProto;
class ConnectionProto;
}

class AbstractTcpChannelFactory;
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
    TcpChannel(uv_stream_t* stream, AbstractTcpChannelFactory* factory);
    void Update();
    void Close();

    virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done);


private:
    virtual int32_t HandleRequest(const proto::ControllerProto& controller_proto);

private: //
    void AfterHandleRequest(google::protobuf::RpcController* controller, google::protobuf::Message* request, google::protobuf::Message* response);

    void* ResultCall(const google::protobuf::MethodDescriptor* method, Controller* controller, const google::protobuf::Message* request, google::protobuf::Message* response, google::protobuf::Closure* done, const proto::ControllerProto&, void*);

private:
    static void AfterWrite(uv_write_t* req, int32_t status);
    static void OnWrite(uv_async_t* handle);
    static void OnRead(uv_stream_t* w, ssize_t nread, const uv_buf_t* buf);
    static void OnAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
    static void OnHandle(uv_async_t* handle);
    static void OnTimer(uv_timer_t* timer);
    static void OnCloseStream(uv_handle_t* handle);

private: // unit test only
    inline const AbstractTcpChannelFactory* factory() const
    {
        return factory_;
    }

private:
    uv_stream_t* stream_;

    // closure
    google::protobuf::RepeatedField<google::protobuf::Closure*> queue_closure_;

    google::protobuf::Map<int64_t, google::protobuf::ResultCallback2<void*, const proto::ControllerProto&, void*>* > result_callback_;

    // write
    uv_mutex_t write_buffer_mutex_;
    uv_async_t write_handle_;
    std::string write_buffer_;
    std::string write_buffer_back_;

    // read
    google::protobuf::Map<const char*, std::string*> reading_buffer_;
    RingInputStream read_stream_;
    int32_t buffer_length_;

    // handle
    uv_mutex_t read_proto_mutex_;
    uv_async_t proto_handle_;
    google::protobuf::RepeatedPtrField<proto::ControllerProto> read_proto_;
    google::protobuf::RepeatedPtrField<proto::ControllerProto> read_proto_back_;

private:
    AbstractTcpChannelFactory* factory_;
    ClosureComponent closure_component_;

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
