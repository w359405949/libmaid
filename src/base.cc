#include "base.h"

namespace maid {

TcpServer::TcpServer(uv_loop_t* loop)
    :loop_(loop),
    current_index_(0),
    router_(new LocalMapRepoChannel())
{
}

TcpServer::~TcpServer()
{
    delete router_;
}

void TcpServer::Close()
{
    for (auto acceptor_it : acceptor_) {
        acceptor_it.second->Close();
        delete acceptor_it.second;
    }
    acceptor_.clear();

    router_->Close();

    uv_stop(mutable_loop());
}

int32_t TcpServer::Listen(const char* host, int32_t port, int32_t backlog)
{
    while (acceptor_.find(current_index_) != acceptor_.end()) {
        current_index_++;
    }

    Acceptor* acceptor = new Acceptor(mutable_loop(), router_);
    acceptor->AddConnectedCallback(std::bind(&TcpServer::ConnectedCallback, this, current_index_, std::placeholders::_1));
    acceptor->AddDisconnectedCallback(std::bind(&TcpServer::DisconnectedCallback, this, current_index_, std::placeholders::_1));
    acceptor_[current_index_] = acceptor;

    return acceptor->Listen(host, port, backlog);
}

void TcpServer::ServeForever()
{
    uv_run(mutable_loop(), UV_RUN_DEFAULT);
}

void TcpServer::Update()
{
    uv_run(mutable_loop(), UV_RUN_NOWAIT);
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
}

void TcpServer::InsertService(google::protobuf::Service* service)
{
    router_->Insert(service);
}

google::protobuf::RpcChannel* TcpServer::channel(int64_t channel_id)
{
    for (auto acceptor_it : acceptor_) {
        google::protobuf::RpcChannel* chl = acceptor_it.second->channel(channel_id);
        if (chl != maid::Channel::default_instance()) {
            return chl;
        }
    }

    return maid::Channel::default_instance();
}

/*
 *
 *
 *
 */

TcpClient::TcpClient(uv_loop_t* loop)
    :loop_(loop),
    current_index_(0),
    router_(new LocalMapRepoChannel())
{
}

TcpClient::~TcpClient()
{
    delete router_;
}

void TcpClient::Close()
{
    for (auto connector_it : connector_) {
        connector_it.second->Close();
        delete connector_it.second;
    }

    connector_.clear();
    router_->Close();

    uv_stop(mutable_loop());
}

int32_t TcpClient::Connect(const char* host, int32_t port)
{
    while (connector_.find(current_index_) != connector_.end()) {
        current_index_++;
    }

    Connector* connector = new Connector(mutable_loop(), router_);
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

    Connector* connector = connector_[index];
    connector->Close();
    delete connector;
    connector_.erase(index);

    current_index_ = index;
}

void TcpClient::InsertService(google::protobuf::Service* service)
{
    router_->Insert(service);
}

void TcpClient::ServeForever()
{
    uv_run(mutable_loop(), UV_RUN_DEFAULT);
}

void TcpClient::Update()
{
    uv_run(mutable_loop(), UV_RUN_NOWAIT);
}

google::protobuf::RpcChannel* TcpClient::channel() const
{
    for (auto connector_it : connector_) {
        if (connector_it.second->channel() != maid::Channel::default_instance()) {
            return connector_it.second->channel();
        }
    }

    return maid::Channel::default_instance();
}

}
