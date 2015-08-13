#include <glog/logging.h>
#include "channel_factory.h"
#include "channel.h"
#include "controller.h"
#include "closure.h"

namespace maid {

/*
 * AbstractTcpChannelFactory
 *
 */

AbstractTcpChannelFactory::AbstractTcpChannelFactory(uv_loop_t* loop, google::protobuf::RpcChannel* router)
    :router_channel_(router),
    loop_(loop)
{
    gc_.data = this;
    uv_prepare_init(loop_, &gc_);
    uv_prepare_start(&gc_, OnGC);
}

AbstractTcpChannelFactory::~AbstractTcpChannelFactory()
{
    router_channel_ = nullptr;
}


void AbstractTcpChannelFactory::Connected(TcpChannel* channel)
{
    channel_[channel] = channel;

    DLOG(INFO)<<"connected:"<< channel_.size();
}

void AbstractTcpChannelFactory::Disconnected(TcpChannel* channel)
{
    CHECK(channel_.find(channel) != channel_.end());

    channel_.erase(channel);
    channel_invalid_.Add(channel);

    channel->Close();
    DLOG(INFO)<<"disconnected:"<< channel_.size();
}

void AbstractTcpChannelFactory::OnGC(uv_prepare_t* handle)
{
    AbstractTcpChannelFactory* self = (AbstractTcpChannelFactory*)handle->data;

    for (auto& channel_it : self->channel_invalid_) {
        delete channel_it;
    }

    self->channel_invalid_.Clear();
}

void AbstractTcpChannelFactory::Close()
{
    uv_prepare_stop(&gc_);
    OnGC(&gc_);
}

google::protobuf::RpcChannel* AbstractTcpChannelFactory::channel(int64_t channel_id)
{
    TcpChannel* key = (TcpChannel*)channel_id;
    const auto& channel_it = channel_.find(key);
    if (channel_it != channel_.end()) {
        return channel_it->second;
    }

    return maid::Channel::default_instance();
}

/*
 *
 * Acceptor
 *
 *
 */
Acceptor::Acceptor(uv_loop_t* loop, google::protobuf::RpcChannel* router)
    :AbstractTcpChannelFactory(loop, router),
    loop_(loop),
    handle_(nullptr)
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
    CHECK(handle_ == nullptr); // Listen only call one time

    handle_ = new uv_tcp_t();
    handle_->data = this;

    int32_t result = 0;
    struct sockaddr_in address;
    result = uv_ip4_addr(host, port, &address);
    if (result) {
        DLOG(ERROR) << uv_strerror(result);
        return result;
    }

    uv_tcp_init(loop_, handle_);

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
    AbstractTcpChannelFactory::Close();

    if (nullptr != handle_) {
        uv_close((uv_handle_t*)handle_, OnCloseListen);
        handle_ = nullptr;
    }
}

void Acceptor::OnAccept(uv_stream_t* stream, int32_t status)
{
    if (status) {
        return;
    }
    Acceptor* self = (Acceptor*)stream->data;

    uv_tcp_t* peer_stream = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    if (nullptr == peer_stream) {
        DLOG(WARNING) << " no more memory, denied";
        return;
    }

    uv_tcp_init(self->loop_, peer_stream);
    int result = uv_accept(stream, (uv_stream_t*)peer_stream);
    if (result) {
        DLOG(WARNING) << uv_strerror(result);
        peer_stream->data = stream->data;
        uv_close((uv_handle_t*)peer_stream, OnCloseListen);
        return;
    }
    uv_tcp_nodelay(peer_stream, 1);

    TcpChannel* channel = nullptr;
    try{
        channel = new TcpChannel(self->loop_, (uv_stream_t*)peer_stream, self);
    } catch (std::bad_alloc) {
        uv_close((uv_handle_t*)peer_stream, OnCloseListen);
        return;
    }

    self->Connected(channel);
}

/*
 *
 * Connector
 *
 */
Connector::Connector(uv_loop_t* loop, google::protobuf::RpcChannel* router)
    :AbstractTcpChannelFactory(loop, router),
    loop_(loop),
    req_(nullptr),
    channel_(nullptr)
{
}

Connector::~Connector()
{
    req_ = nullptr;
    channel_ = nullptr;
}

void Connector::OnCloseHandle(uv_handle_t* handle)
{
    free(handle);
}

void Connector::OnConnect(uv_connect_t* req, int32_t status)
{
    uv_stream_t* stream = req->handle;
    Connector* self = (Connector*)req->data;
    CHECK(self != nullptr);

    if (status) {
        uv_close((uv_handle_t*)stream, OnCloseHandle);
        DLOG(WARNING) << uv_strerror(status);
        return;
    }

    TcpChannel* channel = nullptr;
    try {
        channel = new TcpChannel(self->loop_, stream, self);
    } catch (std::bad_alloc) {
        uv_close((uv_handle_t*)stream, OnCloseHandle);
        return;
    }

    self->Connected(channel);
}

int32_t Connector::Connect(const char* host, int32_t port)
{
    CHECK(nullptr == req_); // only be called one time
    req_ = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    if (req_ == nullptr) {
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
    if (handle == nullptr) {
        return -1;
    }
    uv_tcp_init(loop_, handle);
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
    AbstractTcpChannelFactory::Close();

    free(req_);
    req_ = nullptr;
    channel_ = nullptr;
    loop_ = nullptr;
}

void Connector::Connected(TcpChannel* channel)
{
    CHECK(channel_ == nullptr);
    channel_ = channel;
    AbstractTcpChannelFactory::Connected(channel);
}

void Connector::Disconnected(TcpChannel* channel)
{
    CHECK(channel_ == channel);
    if (channel_ != nullptr) {
        channel_ = nullptr;
    }

    AbstractTcpChannelFactory::Disconnected(channel);
}

google::protobuf::RpcChannel* Connector::channel()
{
    if (channel_ != nullptr) {
        return channel_;
    }

    return maid::Channel::default_instance();
}

} // maid
