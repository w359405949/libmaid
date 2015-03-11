#pragma once
#include <google/protobuf/service.h>
#include <uv.h>
#include <map>
#include <stack>

namespace maid {
namespace proto {
class ConnectionProto;
}

class Channel;
class TcpChannel;
class Controller;
class ChannelPool;
class Closure;

class AbstractTcpChannelFactory
{
public:
    google::protobuf::RpcChannel* router_channel();
    google::protobuf::RpcChannel* middleware_channel();

    virtual void Connected(TcpChannel* channel);
    virtual void Disconnected(TcpChannel* channel);

    AbstractTcpChannelFactory(google::protobuf::RpcChannel* router,
            google::protobuf::RpcChannel* middleware,
            ChannelPool* pool);
    AbstractTcpChannelFactory();
    virtual ~AbstractTcpChannelFactory();

    ChannelPool* pool();

    static AbstractTcpChannelFactory* default_instance();

private:
    Controller* controller_;
    proto::ConnectionProto* connection_;
    Closure* closure_;

    google::protobuf::RpcChannel* router_channel_;
    google::protobuf::RpcChannel* middleware_channel_;
    ChannelPool* pool_;

    friend void InitDefaultInstance();
    static AbstractTcpChannelFactory* default_instance_;
};


class Acceptor : public AbstractTcpChannelFactory
{
public:
    Acceptor(google::protobuf::RpcChannel* router,
            google::protobuf::RpcChannel* middleware,
            ChannelPool* pool);
    Acceptor();

    ~Acceptor();


    int32_t Listen(const char* host, int32_t port, int32_t backlog=1);
    void Close();

    void Connected(TcpChannel* channel);
    void Disconnected(TcpChannel* channel);

public:
    static void OnAccept(uv_stream_t* stream, int32_t status);
    static void OnCloseListen(uv_handle_t* handle);

public: // unit test only
    inline const uv_tcp_t* handle() const
    {
        return handle_;
    }


    inline uint32_t channel_size() const
    {
        return channel_.size();
    }

private:
    uv_tcp_t* handle_;
    std::map<TcpChannel*, TcpChannel*> channel_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Acceptor);
};


class Connector : public AbstractTcpChannelFactory
{
public:
    Connector(google::protobuf::RpcChannel* router,
            google::protobuf::RpcChannel* middleware,
            ChannelPool* pool);
    Connector();
    ~Connector();

    void Connected(TcpChannel* channel);
    void Disconnected(TcpChannel* channel);

    int32_t Connect(const char* host, int32_t port);
    google::protobuf::RpcChannel* channel();
    void Close();

public:
    static void OnConnect(uv_connect_t* req, int32_t status);
    static void OnCloseStream(uv_handle_t* stream);
    static void OnGC(uv_prepare_t* handle);

public:
    inline const uv_connect_t* req() const
    {
        return req_;
    }

private:
    uv_connect_t* req_;
    TcpChannel* channel_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Connector);
};

} // maid
