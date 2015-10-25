#pragma once
#include <functional>
#include <google/protobuf/service.h>
#include <google/protobuf/map.h>
#include <google/protobuf/repeated_field.h>
#include <map>
#include <vector>
#include <uv.h>

namespace maid {
namespace proto {
class ConnectionProto;
}

class Channel;
class TcpChannel;
class Controller;

class AbstractTcpChannelFactory
{
public:
    AbstractTcpChannelFactory(uv_loop_t* loop, google::protobuf::RpcChannel* router);
    virtual ~AbstractTcpChannelFactory();
    virtual void Close();
    virtual void Update();

    google::protobuf::RpcChannel* router_channel()
    {
        return router_channel_;
    }

    google::protobuf::RpcChannel* channel(int64_t channel_id);
    google::protobuf::RpcChannel* channel();

    virtual void Connected(TcpChannel* channel);
    virtual void Disconnected(TcpChannel* channel);

    inline void ConnectedCallback(std::function<void(int64_t)> callback)
    {
        connected_callback_ = callback;
    }

    inline void DisconnectedCallback(std::function<void(int64_t)> callback)
    {
        disconnected_callback_ = callback;
    }

    inline void CloseCallback(std::function<void()> callback)
    {
        close_callback_ = callback;
    }

    void QueueChannel(TcpChannel* channel);
    void QueueChannelInvalid(TcpChannel* channel_invalid);

protected:
    inline uv_loop_t* inner_loop()
    {
        return inner_loop_;
    }

    virtual void InnerCallback();

protected:
    static void OnUpdate(uv_idle_t* handle);
    static void OnWork(uv_work_t* req);
    static void OnAfterWork(uv_async_t* handle);
    static void OnInnerCallback(uv_async_t* handle);
    static void OnFinishWork(uv_work_t* work, int status);
    static void OnCloseInnerLoop(uv_async_t* handle);
    static void OnCloseFactory(uv_async_t* handle);
    static void OnCloseHandle(uv_handle_t* handle);

protected:
    uv_async_t inner_loop_callback_;

private:
    uv_loop_t* loop_;
    uv_idle_t update_;
    uv_async_t* after_work_async_;
    google::protobuf::RpcChannel* router_channel_;

    uv_async_t* close_factory_;
    uv_mutex_t close_factory_mutex_;

    uv_loop_t* inner_loop_;
    uv_mutex_t inner_loop_mutex_;
    uv_sem_t inner_loop_sem_;
    uv_async_t close_inner_loop_;

    uv_mutex_t queue_channel_mutex_;
    google::protobuf::RepeatedField<TcpChannel*> queue_channel_;

    uv_mutex_t channel_invalid_mutex_;
    google::protobuf::RepeatedField<TcpChannel*> channel_invalid_;

    google::protobuf::Map<TcpChannel*, TcpChannel*> channel_;

    std::function<void(int64_t)> connected_callback_;
    std::function<void(int64_t)> disconnected_callback_;
    std::function<void()> close_callback_;
};


class Acceptor : public AbstractTcpChannelFactory
{
public:
    Acceptor(uv_loop_t* loop, google::protobuf::RpcChannel* router);

    ~Acceptor();

    int32_t Listen(const std::string& host, int32_t port);

    virtual void InnerCallback();

public:
    static void OnAccept(uv_stream_t* stream, int32_t status);

private:
    uv_tcp_t* handle_;

    uv_mutex_t address_mutex_;
    struct sockaddr_in address_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Acceptor);
};


class Connector : public AbstractTcpChannelFactory
{
public:
    Connector(uv_loop_t* loop, google::protobuf::RpcChannel* router);
    ~Connector();

    int32_t Connect(const std::string& host, int32_t port);

    virtual void InnerCallback();

public:
    static void OnConnect(uv_connect_t* req, int32_t status);

private:
    uv_mutex_t address_mutex_;
    struct sockaddr_in address;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Connector);
};

} // maid
