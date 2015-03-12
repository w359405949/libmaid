#include <glog/logging.h>
#include "maid/connection.pb.h"
#include "maid/middleware.pb.h"
#include "channel_factory.h"
#include "channel_pool.h"
#include "channel.h"
#include "controller.h"
#include "closure.h"
#include "uv_hook.h"

namespace maid {

AbstractTcpChannelFactory* AbstractTcpChannelFactory::default_instance_ = NULL;

void InitDefaultInstance()
{
    AbstractTcpChannelFactory::default_instance_ = new AbstractTcpChannelFactory();
}

struct StaticInitDefaultInstance
{
    StaticInitDefaultInstance()
    {
        InitDefaultInstance();
    }
} static_init_default_instance_;


/*
 * AbstractTcpChannelFactory
 *
 */

AbstractTcpChannelFactory::AbstractTcpChannelFactory(google::protobuf::RpcChannel* router, google::protobuf::RpcChannel* middleware, ChannelPool* pool)
    :router_channel_(router),
    middleware_channel_(middleware),
    pool_(pool)
{
    controller_ = new Controller();
    connection_ = new proto::ConnectionProto();
    closure_ = new Closure();
}


AbstractTcpChannelFactory* AbstractTcpChannelFactory::default_instance()
{
    return default_instance_;
}

AbstractTcpChannelFactory::AbstractTcpChannelFactory()
    :router_channel_(NULL),
    middleware_channel_(NULL),
    pool_(NULL)
{
    controller_ = new Controller();
    connection_ = new proto::ConnectionProto();
    closure_ = new Closure();
}

ChannelPool* AbstractTcpChannelFactory::pool()
{
    return pool_ == NULL ? ChannelPool::generated_pool() : pool_;
}

AbstractTcpChannelFactory::~AbstractTcpChannelFactory()
{
    router_channel_ = NULL;
    middleware_channel_ = NULL;
    pool_ = NULL;

    delete controller_;
    delete connection_;
    delete closure_;
}

google::protobuf::RpcChannel* AbstractTcpChannelFactory::router_channel()
{
    router_channel_ = pool()->channel(router_channel_);
    return router_channel_;
}

google::protobuf::RpcChannel* AbstractTcpChannelFactory::middleware_channel()
{
    middleware_channel_ = pool()->channel(middleware_channel_);
    return middleware_channel_;
}

void AbstractTcpChannelFactory::Connected(TcpChannel* channel)
{
    pool()->AddChannel(channel);

    connection_->set_id((int64_t)channel->stream());
    proto::Middleware_Stub stub(middleware_channel());
    stub.Connected(controller_, connection_, connection_, closure_);
}

void AbstractTcpChannelFactory::Disconnected(TcpChannel* channel)
{
    pool()->RemoveChannel(channel);

    connection_->set_id((int64_t)channel->stream());
    proto::Middleware_Stub stub(middleware_channel());
    stub.Disconnected(controller_, connection_, connection_, closure_);

}

/*
 *
 * Acceptor
 *
 *
 */
Acceptor::Acceptor(google::protobuf::RpcChannel* router, google::protobuf::RpcChannel* middleware, ChannelPool* pool)
    :AbstractTcpChannelFactory(router, middleware, pool),
    handle_(NULL)
{
}

Acceptor::Acceptor()
    :handle_(NULL)
{
}

Acceptor::~Acceptor()
{
    delete handle_;
}

void Acceptor::OnCloseListen(uv_handle_t* handle)
{
    delete handle;
}

int32_t Acceptor::Listen(const char* host, int32_t port, int32_t backlog)
{
    CHECK(handle_ == NULL); // Listen only call one time

    handle_ = new uv_tcp_t();
    handle_->data = this;

    int32_t result = 0;
    struct sockaddr_in address;
    result = uv_ip4_addr(host, port, &address);
    if (result) {
        DLOG(ERROR) << uv_strerror(result);
        return result;
    }

    uv_tcp_init(maid_default_loop(), handle_);

    int32_t flags = 0;
    result = uv_tcp_bind(handle_, (struct sockaddr*)&address, flags);
    if (result) {
        Close();
        DLOG(ERROR) << uv_strerror(result);
        return result;
    }

    result = uv_listen((uv_stream_t*)handle_, backlog, OnAccept);
    if (result) {
        Close();
        DLOG(ERROR) << uv_strerror(result);
        return result;
    }

    return result;
}

void Acceptor::Close()
{
    std::map<TcpChannel*, TcpChannel*>::iterator it;
    for (it = channel_.begin(); it != channel_.end(); it++) {
        it->second->Close();
        AbstractTcpChannelFactory::Disconnected(it->second);
    }
    channel_.clear();

    if (NULL != handle_) {
        uv_close((uv_handle_t*)handle_, OnCloseListen);
        handle_ = NULL;
    }
}

void Acceptor::OnAccept(uv_stream_t* stream, int32_t status)
{
    if (status) {
        return;
    }
    Acceptor* self = (Acceptor*)stream->data;

    uv_tcp_t* peer_stream = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    if (NULL == peer_stream) {
        DLOG(WARNING) << " no more memory, denied";
        return;
    }

    uv_tcp_init(maid_default_loop(), peer_stream);
    int result = uv_accept(stream, (uv_stream_t*)peer_stream);
    if (result) {
        DLOG(WARNING) << uv_strerror(result);
        peer_stream->data = stream->data;
        uv_close((uv_handle_t*)peer_stream, OnCloseListen);
        return;
    }
    uv_tcp_nodelay(peer_stream, 1);

    TcpChannel* channel = NULL;
    try{
        channel = new TcpChannel((uv_stream_t*)peer_stream, self);
    } catch (std::bad_alloc) {
        uv_close((uv_handle_t*)peer_stream, OnCloseListen);
        return;
    }

    self->Connected(channel);
}

void Acceptor::Connected(TcpChannel* channel)
{
    CHECK(channel_.find(channel) == channel_.end());
    channel_[channel] = channel;
    AbstractTcpChannelFactory::Connected(channel);

    DLOG(INFO)<<"connected:"<< channel_.size();
}

void Acceptor::Disconnected(TcpChannel* channel)
{
    CHECK(channel_.find(channel) != channel_.end());

    AbstractTcpChannelFactory::Disconnected(channel);
    channel->Close();
    channel_[channel] = NULL;
    channel_.erase(channel);

    DLOG(INFO)<<"connected:"<< channel_.size();
}


/*
 *
 * Connector
 *
 */
Connector::Connector(google::protobuf::RpcChannel* router, google::protobuf::RpcChannel* middleware, ChannelPool* pool)
    :AbstractTcpChannelFactory(router, middleware, pool),
    req_(NULL),
    channel_(NULL)
{
}

Connector::Connector()
    :req_(NULL),
    channel_(NULL)
{
}

Connector::~Connector()
{
    req_ = NULL;
    channel_ = NULL;
}

void Connector::OnCloseHandle(uv_handle_t* handle)
{
    free(handle);
}

void Connector::OnConnect(uv_connect_t* req, int32_t status)
{
    uv_stream_t* stream = req->handle;
    Connector* self = (Connector*)req->data;
    CHECK(self != NULL);

    if (status) {
        uv_close((uv_handle_t*)stream, OnCloseHandle);
        DLOG(WARNING) << uv_strerror(status);
        return;
    }

    TcpChannel* channel = NULL;
    try {
        channel = new TcpChannel(stream, self);
    } catch (std::bad_alloc) {
        uv_close((uv_handle_t*)stream, OnCloseHandle);
        return;
    }

    self->Connected(channel);
}

int32_t Connector::Connect(const char* host, int32_t port)
{
    CHECK(NULL == req_); // only be called one time
    req_ = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    if (req_ == NULL) {
        return -1;
    }
    req_->data = this;

    struct sockaddr_in address;
    int result = 0;
    result = uv_ip4_addr(host, port, &address);
    if (result) {
        DLOG(WARNING) << uv_strerror(result);
        return result;
    }

    uv_tcp_t* handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    if (handle == NULL) {
        return -1;
    }
    uv_tcp_init(maid_default_loop(), handle);
    uv_tcp_nodelay(handle, 1);
    result = uv_tcp_connect(req_, handle, (struct sockaddr*)&address, OnConnect);
    if (result) {
        DLOG(WARNING) << uv_strerror(result);
        uv_close((uv_handle_t*)handle, OnCloseHandle);
        Close();
        return result;
    }

    return result;
}

void Connector::Close()
{
    Disconnected(channel_);

    if (NULL != req_) {
        free(req_);
    }
}

void Connector::Connected(TcpChannel* channel)
{
    CHECK(channel_ == NULL);
    channel_ = channel;
    AbstractTcpChannelFactory::Connected(channel);
}

void Connector::Disconnected(TcpChannel* channel)
{
    CHECK(channel_ == channel);
    if (channel_ != NULL) {
        channel_ = NULL;
        channel->Close();
        AbstractTcpChannelFactory::Disconnected(channel);
    }
}

google::protobuf::RpcChannel* Connector::channel()
{
    if (channel_ == NULL) {
        return maid::Channel::default_instance();
    }
    return channel_;
}

} // maid
