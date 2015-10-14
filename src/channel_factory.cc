#include <google/protobuf/stubs/logging.h>
#include "channel_factory.h"
#include "channel.h"
#include "controller.h"

namespace maid {

/*
 * AbstractTcpChannelFactory
 *
 */

AbstractTcpChannelFactory::AbstractTcpChannelFactory(uv_loop_t* loop, google::protobuf::RpcChannel* router)
    :loop_(loop),
    router_channel_(router)
{
    inner_loop_ = (uv_loop_t*)malloc(sizeof(uv_loop_t));
    uv_loop_init(inner_loop_);

    uv_mutex_init(&queue_channel_mutex_);
    queue_channel_async_.data = this;
    uv_async_init(loop_, &queue_channel_async_, OnQueueChannel);

    uv_mutex_init(&channel_invalid_mutex_);
    channel_invalid_async_.data = this;
    uv_async_init(loop_, &channel_invalid_async_, OnChannelInvalid);

    uv_mutex_init(&inner_loop_lock_);
    inner_loop_callback_.data = this;
    uv_async_init(inner_loop_, &inner_loop_callback_, OnInnerCallback);

    close_inner_loop_.data = this;
    uv_async_init(inner_loop_, &close_inner_loop_, OnCloseInnerLoop);

    work_.data = this;
    uv_queue_work(loop_, &work_, OnWork, OnAfterWork);
}

AbstractTcpChannelFactory::~AbstractTcpChannelFactory()
{
    loop_ = nullptr;
    router_channel_ = nullptr;

    uv_loop_close(inner_loop_);
    free(inner_loop_);
}


void AbstractTcpChannelFactory::Close()
{
    uv_cancel((uv_req_t*)&work_);

    uv_async_send(&close_inner_loop_);
    uv_mutex_lock(&inner_loop_lock_); // block until inner loop stop
    uv_mutex_unlock(&inner_loop_lock_);
}

void AbstractTcpChannelFactory::Connected(TcpChannel* channel)
{
    channel_[channel] = channel;

    for (auto& function : connected_callbacks_) {
        function((int64_t)channel);
    }
}

void AbstractTcpChannelFactory::Disconnected(TcpChannel* channel)
{
    channel_.erase(channel);

    for (auto& function : disconnected_callbacks_) {
        function((int64_t)channel);
    }
}

void AbstractTcpChannelFactory::QueueChannel(TcpChannel* channel)
{
    uv_mutex_lock(&queue_channel_mutex_);
    queue_channel_.Add(channel);
    uv_mutex_unlock(&queue_channel_mutex_);

    uv_async_send(&queue_channel_async_);
}

void AbstractTcpChannelFactory::QueueChannelInvalid(TcpChannel* channel_invalid)
{
    if (channel_invalid != nullptr) {
        channel_invalid->Close();
    }

    uv_mutex_lock(&channel_invalid_mutex_);
    channel_invalid_.Add(channel_invalid);
    uv_mutex_unlock(&channel_invalid_mutex_);

    uv_async_send(&channel_invalid_async_);
}

void AbstractTcpChannelFactory::OnQueueChannel(uv_async_t* handle)
{
    AbstractTcpChannelFactory* self = (AbstractTcpChannelFactory*)handle->data;

    uv_mutex_lock(&self->queue_channel_mutex_);
    self->queue_channel_.Swap(&self->queue_channel_back_);
    uv_mutex_unlock(&self->queue_channel_mutex_);

    for (auto& channel : self->queue_channel_back_) {
        self->Connected(channel);
    }

    self->queue_channel_back_.Clear();
}

void AbstractTcpChannelFactory::OnChannelInvalid(uv_async_t* handle)
{
    AbstractTcpChannelFactory* self = (AbstractTcpChannelFactory*)handle->data;

    uv_mutex_lock(&self->channel_invalid_mutex_);
    self->channel_invalid_.Swap(&self->channel_invalid_back_);
    uv_mutex_unlock(&self->channel_invalid_mutex_);

    for (auto& channel_invalid : self->channel_invalid_back_) {
        self->Disconnected(channel_invalid);
        delete channel_invalid;
    }

    self->channel_invalid_back_.Clear();
}

void AbstractTcpChannelFactory::OnWork(uv_work_t* handle)
{
    AbstractTcpChannelFactory* self = (AbstractTcpChannelFactory*)handle->data;

    uv_mutex_lock(&self->inner_loop_lock_);
    uv_run(self->inner_loop_, UV_RUN_DEFAULT);
    uv_mutex_unlock(&self->inner_loop_lock_);
}

void AbstractTcpChannelFactory::OnAfterWork(uv_work_t* handle, int32_t statu)
{
}

void AbstractTcpChannelFactory::OnCloseInnerLoop(uv_async_t* handle)
{
    AbstractTcpChannelFactory* self = (AbstractTcpChannelFactory*)handle->data;

    uv_stop(self->inner_loop_);
}

void AbstractTcpChannelFactory::OnUpdate(uv_idle_t* handle)
{
    AbstractTcpChannelFactory* self = (AbstractTcpChannelFactory*)handle->data;

    self->Update();
}

void AbstractTcpChannelFactory::OnInnerCallback(uv_async_t* handle)
{
    AbstractTcpChannelFactory* self = (AbstractTcpChannelFactory*)handle->data;

    self->InnerCallback();
}

void AbstractTcpChannelFactory::Update()
{
    for (auto& channel : channel_) {
        channel.second->Update();
    }
}

void AbstractTcpChannelFactory::InnerCallback()
{
}

google::protobuf::RpcChannel* AbstractTcpChannelFactory::channel(int64_t channel_id)
{
    google::protobuf::RpcChannel* chl = maid::Channel::default_instance();

    TcpChannel* key = (TcpChannel*)channel_id;
    const auto& channel_it = channel_.find(key);
    if (channel_it != channel_.end()) {
        chl = channel_it->second;
    }

    return chl;
}


google::protobuf::RpcChannel* AbstractTcpChannelFactory::channel()
{
    google::protobuf::RpcChannel* chl = maid::Channel::default_instance();

    if (!channel_.empty()) {
        chl = channel_.begin()->second;
    }

    return chl;
}

/*
 *
 * Acceptor
 *
 *
 */
Acceptor::Acceptor(uv_loop_t* loop, google::protobuf::RpcChannel* router)
    :AbstractTcpChannelFactory(loop, router),
    handle_(nullptr)
{
    uv_mutex_init(&address_mutex_);
}

Acceptor::~Acceptor()
{
    free(handle_);
}

void Acceptor::OnCloseHandle(uv_handle_t* handle)
{
    free(handle);
}

int32_t Acceptor::Listen(const std::string& host, int32_t port)
{
    int32_t result = 0;

    uv_mutex_lock(&address_mutex_);
    result = uv_ip4_addr(host.c_str(), port, &address_);
    uv_mutex_unlock(&address_mutex_);
    GOOGLE_LOG_IF(WARNING, result != 0) << uv_strerror(result);

    uv_async_send(&inner_loop_callback_);

    return result;
}

void Acceptor::InnerCallback()
{
    GOOGLE_CHECK(handle_ == nullptr) << "acceptor listen call twice";
    int32_t result = 0;

    handle_ = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    handle_->data = this;

    uv_tcp_init(inner_loop(), handle_);
    uv_tcp_nodelay(handle_, 1);

    uv_mutex_lock(&address_mutex_);
    result = uv_tcp_bind(handle_, (struct sockaddr*)&address_, 0);
    uv_mutex_unlock(&address_mutex_);

    if (result) {
        GOOGLE_LOG(WARNING) << uv_strerror(result);
        uv_async_send(&close_inner_loop_);
        return;
    }

    result = uv_listen((uv_stream_t*)handle_, 65535, OnAccept);
    if (result) {
        GOOGLE_LOG(WARNING) << uv_strerror(result);
        uv_async_send(&close_inner_loop_);
        return;
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
        GOOGLE_LOG(WARNING) << " no more memory, denied";
        return;
    }

    uv_tcp_init(stream->loop, peer_stream);
    int result = uv_accept(stream, (uv_stream_t*)peer_stream);
    if (result) {
        GOOGLE_LOG(WARNING) << uv_strerror(result);
        uv_close((uv_handle_t*)peer_stream, OnCloseHandle);
        return;
    }

    self->QueueChannel(new TcpChannel((uv_stream_t*)peer_stream, self));
}

/*
 *
 * Connector
 *
 */
Connector::Connector(uv_loop_t* loop, google::protobuf::RpcChannel* router)
    :AbstractTcpChannelFactory(loop, router),
    req_(nullptr)
{
    uv_mutex_init(&address_mutex_);
}

Connector::~Connector()
{
    free(req_);
    req_ = nullptr;
}

void Connector::OnCloseHandle(uv_handle_t* handle)
{
    free(handle);
}

void Connector::OnConnect(uv_connect_t* req, int32_t status)
{
    uv_stream_t* stream = req->handle;
    Connector* self = (Connector*)req->data;

    if (status) {
        GOOGLE_LOG(WARNING) << uv_strerror(status);
        self->QueueChannelInvalid(NULL);
        return;
    }

    self->QueueChannel(new TcpChannel(stream, self));
}

int32_t Connector::Connect(const std::string& host, int32_t port)
{
    int result = 0;
    uv_mutex_lock(&address_mutex_);
    result = uv_ip4_addr(host.c_str(), port, &address);
    uv_mutex_unlock(&address_mutex_);
    GOOGLE_LOG_IF(WARNING, result != 0) << uv_strerror(result);

    uv_async_send(&inner_loop_callback_);

    return result;
}

void Connector::InnerCallback()
{
    GOOGLE_CHECK(req_ == nullptr) << "Connector::Connect called twice";
    int result = 0;

    req_ = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    if (req_ == nullptr) {
        uv_async_send(&close_inner_loop_);
        return;
    }
    req_->data = this;

    uv_tcp_t* handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    if (handle == nullptr) {
        uv_async_send(&close_inner_loop_);
        return;
    }

    uv_tcp_init(inner_loop(), handle);
    uv_mutex_lock(&address_mutex_);
    result = uv_tcp_connect(req_, handle, (struct sockaddr*)&address, OnConnect);
    uv_mutex_unlock(&address_mutex_);
    if (result) {
        GOOGLE_LOG(WARNING) << uv_strerror(result);
        uv_async_send(&close_inner_loop_);
        return;
    }
}


} // maid
