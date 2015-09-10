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
class Closure;

class AbstractTcpChannelFactory
{
public:
    AbstractTcpChannelFactory(uv_loop_t* loop, google::protobuf::RpcChannel* router);
    virtual ~AbstractTcpChannelFactory();

    google::protobuf::RpcChannel* router_channel()
    {
        return router_channel_;
    }

    google::protobuf::RpcChannel* channel(int64_t channel_id);

    virtual void Connected(TcpChannel* channel);
    virtual void Disconnected(TcpChannel* channel);
    virtual void Close();

    inline void AddConnectedCallback(std::function<void(int64_t)> callback)
    {
        connected_callbacks_.push_back(callback);
    }

    inline void AddDisconnectedCallback(std::function<void(int64_t)> callback)
    {
        disconnected_callbacks_.push_back(callback);
    }

public:
    static void OnGC(uv_prepare_t* handle);

private:
    uv_loop_t* loop_;
    uv_prepare_t gc_;

    google::protobuf::RpcChannel* router_channel_;
    google::protobuf::RepeatedField<TcpChannel*> channel_invalid_;
    google::protobuf::Map<TcpChannel*, TcpChannel*> channel_;

    std::vector<std::function<void(int64_t)> > connected_callbacks_;
    std::vector<std::function<void(int64_t)> > disconnected_callbacks_;
};


class Acceptor : public AbstractTcpChannelFactory
{
public:
    Acceptor(uv_loop_t* loop, google::protobuf::RpcChannel* router);

    ~Acceptor();


    int32_t Listen(const char* host, int32_t port, int32_t backlog=1);
    virtual void Close();

public:
    static void OnAccept(uv_stream_t* stream, int32_t status);
    static void OnCloseListen(uv_handle_t* handle);

public: // unit test only
    inline const uv_tcp_t* handle() const
    {
        return handle_;
    }

private:
    uv_loop_t* loop_;
    uv_tcp_t* handle_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Acceptor);
};


class Connector : public AbstractTcpChannelFactory
{
public:
    Connector(uv_loop_t* loop, google::protobuf::RpcChannel* router);
    ~Connector();

    void Connected(TcpChannel* channel);
    void Disconnected(TcpChannel* channel);

    int32_t Connect(const char* host, int32_t port);
    google::protobuf::RpcChannel* channel();
    void Close();

public:
    static void OnConnect(uv_connect_t* req, int32_t status);
    static void OnCloseHandle(uv_handle_t* handle);
    static void OnGC(uv_prepare_t* handle);

public:
    inline const uv_connect_t* req() const
    {
        return req_;
    }

private:
    uv_loop_t* loop_;
    uv_connect_t* req_;
    TcpChannel* channel_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Connector);
};

} // maid
