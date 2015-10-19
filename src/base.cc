#include "base.h"

namespace maid {

TcpServer::TcpServer()
    :loop_(nullptr),
    current_index_(0),
    router_(new LocalMapRepoChannel())
{
    loop_ = (uv_loop_t*)malloc(uv_loop_size());
    uv_loop_init(loop_);

    gc_.data = this;
    uv_async_init(loop_, &gc_, OnGC);
}

TcpServer::~TcpServer()
{
    uv_loop_close(loop_);
    OnGC(&gc_);
    free(loop_);
    loop_ = nullptr;

    router_->Close();
    delete router_;
    router_ = nullptr;
}

void TcpServer::Close()
{
    google::protobuf::Map<int32_t, Acceptor*> acceptors(acceptor_);
    for (auto acceptor_it : acceptors) {
        acceptor_it.second->Close();
        acceptor_invalid_.Add(acceptor_it.second);
    }
    acceptor_.clear();

    uv_stop(loop_);
}

void TcpServer::OnGC(uv_async_t* handle)
{
    TcpServer* self = (TcpServer*)handle->data;

    for (auto& acceptor : self->acceptor_invalid_) {
        delete acceptor;
    }

    self->acceptor_invalid_.Clear();
}

int32_t TcpServer::Listen(const std::string& host, int32_t port)
{
    while (acceptor_.find(current_index_) != acceptor_.end()) {
        current_index_++;
    }

    Acceptor* acceptor = new Acceptor(loop_, router_);
    acceptor->AddConnectedCallback(std::bind(&TcpServer::ConnectedCallback, this, current_index_, std::placeholders::_1));
    acceptor->AddDisconnectedCallback(std::bind(&TcpServer::DisconnectedCallback, this, current_index_, std::placeholders::_1));
    acceptor_[current_index_] = acceptor;

    return acceptor->Listen(host, port);
}

void TcpServer::ServeForever()
{
    uv_run(loop_, UV_RUN_DEFAULT);
}

void TcpServer::Update()
{
    uv_run(loop_, UV_RUN_NOWAIT);
}


void TcpServer::ConnectedCallback(int32_t index, int64_t connection_id)
{
    for (auto& callback : connected_callbacks_) {
        callback(connection_id);
    }
}

void TcpServer::DisconnectedCallback(int32_t index, int64_t connection_id)
{
    for (auto& callback : disconnected_callbacks_) {
        callback(connection_id);
    }

    if (connection_id != 0) {
        return;
    }
    current_index_ = index;

    Acceptor* acceptor = acceptor_[index];
    acceptor_.erase(index);
    acceptor_invalid_.Add(acceptor);
    uv_async_send(&gc_);
}

void TcpServer::InsertService(google::protobuf::Service* service)
{
    router_->Insert(service);
}

google::protobuf::RpcChannel* TcpServer::channel(int64_t channel_id)
{
    google::protobuf::RpcChannel* chl = maid::Channel::default_instance();
    for (auto acceptor_it : acceptor_) {
        chl = acceptor_it.second->channel(channel_id);
        if (chl != maid::Channel::default_instance()) {
            break;
        }
    }

    return chl;
}

/*
 *
 *
 *
 */

TcpClient::TcpClient()
    :loop_(nullptr),
    current_index_(0),
    router_(new LocalMapRepoChannel())
{
    loop_ = (uv_loop_t*)malloc(uv_loop_size());
    uv_loop_init(loop_);

    gc_.data = this;
    uv_async_init(loop_, &gc_, OnGC);
}

TcpClient::~TcpClient()
{
    uv_loop_close(loop_);
    OnGC(&gc_);
    free(loop_);
    loop_ = nullptr;

    router_->Close();
    delete router_;
    router_ = nullptr;
}

void TcpClient::Close()
{
    google::protobuf::Map<int32_t, Connector*> connectors;
    for (auto connector_it : connectors) {
        connector_it.second->Close();
        connector_invalid_.Add(connector_it.second);
    }
    connector_.clear();

    uv_stop(loop_);
}


void TcpClient::OnGC(uv_async_t* handle)
{
    TcpClient* self = (TcpClient*)handle->data;

    for (auto connector : self->connector_invalid_) {
        delete connector;
    }

    self->connector_invalid_.Clear();
}

int32_t TcpClient::Connect(const std::string& host, int32_t port)
{
    while (connector_.find(current_index_) != connector_.end()) {
        current_index_++;
    }

    Connector* connector = new Connector(loop_, router_);
    connector->AddConnectedCallback(std::bind(&TcpClient::ConnectedCallback, this, current_index_, std::placeholders::_1));
    connector->AddDisconnectedCallback(std::bind(&TcpClient::DisconnectedCallback, this, current_index_, std::placeholders::_1));
    connector_[current_index_] = connector;

    return connector->Connect(host, port);
}

void TcpClient::ConnectedCallback(int32_t index, int64_t connection_id)
{
    for (auto& callback : connected_callbacks_) {
        callback(connection_id);
    }
}

void TcpClient::DisconnectedCallback(int32_t index, int64_t connection_id)
{
    for (auto& callback : disconnected_callbacks_) {
        callback(connection_id);
    }
    current_index_ = index;

    Connector* connector = connector_[index];
    connector_.erase(index);
    connector_invalid_.Add(connector);
    uv_async_send(&gc_);
}

void TcpClient::InsertService(google::protobuf::Service* service)
{
    router_->Insert(service);
}

void TcpClient::ServeForever()
{
    uv_run(loop_, UV_RUN_DEFAULT);
}

void TcpClient::Update()
{
    uv_run(loop_, UV_RUN_NOWAIT);
}

google::protobuf::RpcChannel* TcpClient::channel() const
{
    google::protobuf::RpcChannel* chl = maid::Channel::default_instance();
    for (auto connector_it : connector_) {
        chl = connector_it.second->channel();
        if (chl != maid::Channel::default_instance()) {
            break;
        }
    }

    return chl;
}

}
