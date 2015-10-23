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
    uv_mutex_init(&queue_channel_mutex_);
    uv_mutex_init(&channel_invalid_mutex_);

    after_work_async_ = (uv_async_t*)malloc(sizeof(uv_async_t));
    after_work_async_->data = this;
    uv_async_init(loop_, after_work_async_, OnAfterWork);

    update_.data = this;
    uv_idle_init(loop_, &update_);
    uv_idle_start(&update_, OnUpdate);

    close_factory_ = (uv_async_t*)malloc(sizeof(uv_async_t));
    close_factory_->data = this;
    uv_async_init(loop_, close_factory_, OnCloseFactory);

    // inner
    inner_loop_ = (uv_loop_t*)malloc(uv_loop_size());
    uv_loop_init(inner_loop_);

    uv_sem_init(&inner_loop_sem_, 0);
    uv_mutex_init(&inner_loop_mutex_);
    inner_loop_callback_.data = this;
    uv_async_init(inner_loop_, &inner_loop_callback_, OnInnerCallback);

    close_inner_loop_.data = this;
    uv_async_init(inner_loop_, &close_inner_loop_, OnCloseInnerLoop);

    // queue work

    uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
    work->data = this;
    uv_queue_work(loop_, work, OnWork, OnFinishWork);
}

AbstractTcpChannelFactory::~AbstractTcpChannelFactory()
{
    GOOGLE_CHECK(channel_invalid_.empty());
    GOOGLE_CHECK(queue_channel_.empty());
    GOOGLE_CHECK(channel_.empty()) << channel_.size();

    connected_callbacks_.clear();
    disconnected_callbacks_.clear();

    uv_mutex_destroy(&queue_channel_mutex_);
    uv_mutex_destroy(&channel_invalid_mutex_);
    uv_mutex_destroy(&inner_loop_mutex_);
    uv_sem_destroy(&inner_loop_sem_);

    free(inner_loop_);
    inner_loop_ = nullptr;

    loop_ = nullptr;
    router_channel_ = nullptr;
}

void AbstractTcpChannelFactory::OnClose(uv_handle_t* handle)
{
    free(handle);
}

void AbstractTcpChannelFactory::OnCloseFactory(uv_async_t* handle)
{
    if (uv_is_closing((uv_handle_t*)handle)) {
        return;
    }

    AbstractTcpChannelFactory* self = (AbstractTcpChannelFactory*)handle->data;

    uv_close((uv_handle_t*)self->after_work_async_, OnClose);
    uv_close((uv_handle_t*)self->close_factory_, OnClose);

    uv_async_send(&self->close_inner_loop_);
    uv_sem_wait(&self->inner_loop_sem_);
    uv_mutex_lock(&self->inner_loop_mutex_);
    {
        uv_close((uv_handle_t*)&self->inner_loop_callback_, nullptr);
        uv_close((uv_handle_t*)&self->close_inner_loop_, nullptr);
    }
    uv_mutex_unlock(&self->inner_loop_mutex_);
    uv_loop_close(self->inner_loop_);

    for (auto& channel : self->channel_) {
        channel.second->Close();
    }

    self->close_factory_ = nullptr;
}

void AbstractTcpChannelFactory::Update()
{
    google::protobuf::RepeatedField<TcpChannel*> queue_channel_back;
    uv_mutex_lock(&queue_channel_mutex_);
    {
        queue_channel_back.Swap(&queue_channel_);
    }
    uv_mutex_unlock(&queue_channel_mutex_);

    for (auto& channel : queue_channel_back) {
        Connected(channel);
    }

    for (auto& channel : channel_) {
        channel.second->Update();
    }

    google::protobuf::RepeatedField<TcpChannel*> channel_invalid_back;
    uv_mutex_lock(&channel_invalid_mutex_);
    {
        channel_invalid_back.Swap(&channel_invalid_);
    }
    uv_mutex_unlock(&channel_invalid_mutex_);

    for (auto& channel_invalid : channel_invalid_back) {
        Disconnected(channel_invalid);
        delete channel_invalid;
    }

    if (close_factory_ == nullptr && channel_.empty()) {
        uv_idle_stop(&update_);
        Disconnected(nullptr);
    }
}

void AbstractTcpChannelFactory::Close()
{
    if (close_factory_ != nullptr) {
        uv_async_send(close_factory_);
    }
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
    {
        queue_channel_.Add(channel);
    }
    uv_mutex_unlock(&queue_channel_mutex_);
}

void AbstractTcpChannelFactory::QueueChannelInvalid(TcpChannel* channel_invalid)
{
    uv_mutex_lock(&channel_invalid_mutex_);
    {
        channel_invalid_.Add(channel_invalid);
    }
    uv_mutex_unlock(&channel_invalid_mutex_);
}

void AbstractTcpChannelFactory::OnWork(uv_work_t* handle)
{
    AbstractTcpChannelFactory* self = (AbstractTcpChannelFactory*)handle->data;

    uv_mutex_lock(&self->inner_loop_mutex_);
    {
        uv_sem_post(&self->inner_loop_sem_);
        uv_run(self->inner_loop_, UV_RUN_ONCE);
        uv_async_send(self->after_work_async_);
    }
    uv_mutex_unlock(&self->inner_loop_mutex_);
}

void AbstractTcpChannelFactory::OnCloseInnerLoop(uv_async_t* handle)
{
    AbstractTcpChannelFactory* self = (AbstractTcpChannelFactory*)handle->data;

    uv_stop(self->inner_loop_);
}

void AbstractTcpChannelFactory::OnAfterWork(uv_async_t* handle)
{
    if (uv_is_closing((uv_handle_t*)handle)) {
        return;
    }

    AbstractTcpChannelFactory* self = (AbstractTcpChannelFactory*)handle->data;

    uv_sem_wait(&self->inner_loop_sem_);

    uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
    work->data = self;
    uv_queue_work(self->loop_, work, OnWork, OnFinishWork);
}

void AbstractTcpChannelFactory::OnFinishWork(uv_work_t* req, int status)
{
    free(req);
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
    uv_mutex_destroy(&address_mutex_);
    free(handle_);
    handle_ = nullptr;
}

int32_t Acceptor::Listen(const std::string& host, int32_t port)
{
    int32_t result = 0;

    uv_mutex_lock(&address_mutex_);
    {
        result = uv_ip4_addr(host.c_str(), port, &address_);
    }
    uv_async_send(&inner_loop_callback_);
    uv_mutex_unlock(&address_mutex_);
    GOOGLE_LOG_IF(WARNING, result != 0) << uv_strerror(result);


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
    {
        result = uv_tcp_bind(handle_, (struct sockaddr*)&address_, 0);
    }
    uv_mutex_unlock(&address_mutex_);

    GOOGLE_LOG_IF(WARNING, result != 0) << uv_strerror(result);
    if (result) {
        Close();
        return;
    }

    result = uv_listen((uv_stream_t*)handle_, 65535, OnAccept);
    GOOGLE_LOG_IF(WARNING, result != 0) << uv_strerror(result);
    if (result) {
        Close();
        return;
    }
}

void Acceptor::OnAccept(uv_stream_t* stream, int status)
{
    GOOGLE_LOG_IF(WARNING, status) << uv_strerror(status);
    if (status) {
        return;
    }

    Acceptor* self = (Acceptor*)stream->data;

    uv_tcp_t* peer_stream = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(stream->loop, peer_stream);
    int result = uv_accept(stream, (uv_stream_t*)peer_stream);
    GOOGLE_LOG_IF(WARNING, result != 0) << uv_strerror(result);
    if (result) {
        uv_close((uv_handle_t*)peer_stream, OnClose);
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
    uv_mutex_destroy(&address_mutex_);
    free(req_);
    req_ = nullptr;
}

void Connector::OnConnect(uv_connect_t* req, int32_t status)
{
    uv_stream_t* stream = req->handle;
    Connector* self = (Connector*)req->data;

    GOOGLE_LOG_IF(WARNING, status != 0) << uv_strerror(status);

    if (status) {
        self->Close();
        return;
    }

    self->QueueChannel(new TcpChannel(stream, self));
}

int32_t Connector::Connect(const std::string& host, int32_t port)
{
    int result = 0;
    uv_mutex_lock(&address_mutex_);
    {
        result = uv_ip4_addr(host.c_str(), port, &address);
    }
    uv_async_send(&inner_loop_callback_);
    uv_mutex_unlock(&address_mutex_);

    GOOGLE_LOG_IF(WARNING, result != 0) << uv_strerror(result);

    return result;
}

void Connector::InnerCallback()
{
    GOOGLE_CHECK(req_ == nullptr) << "Connector::Connect called twice";
    int result = 0;

    req_ = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    req_->data = this;

    uv_tcp_t* handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));

    uv_tcp_init(inner_loop(), handle);
    uv_mutex_lock(&address_mutex_);
    {
        result = uv_tcp_connect(req_, handle, (struct sockaddr*)&address, OnConnect);
    }
    uv_mutex_unlock(&address_mutex_);

    GOOGLE_LOG_IF(WARNING, result != 0) << uv_strerror(result);

    if (result) {
        Close();
        return;
    }
}


} // maid
