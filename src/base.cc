#include "base.h"

namespace maid {

TcpServer::TcpServer(uv_loop_t* loop)
    :loop_(loop)
{
    router_ = new LocalMapRepoChannel();
}

TcpServer::~TcpServer()
{
    delete router_;
}

void TcpServer::Close()
{
    for (auto& acceptor : acceptor_) {
        acceptor->Close();
        delete acceptor;
    }
    acceptor_.Clear();

    router_->Close();

    uv_stop(mutable_loop());
}

int32_t TcpServer::Listen(const char* host, int32_t port, int32_t backlog)
{
    Acceptor* acceptor = new Acceptor(mutable_loop(), router_);
    acceptor_.Add(acceptor);
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

void TcpServer::InsertService(google::protobuf::Service* service)
{
    router_->Insert(service);
}

google::protobuf::RpcChannel* TcpServer::channel(int64_t channel_id)
{
    for (auto& acceptor_it : acceptor_) {
        google::protobuf::RpcChannel* chl = acceptor_it->channel(channel_id);
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
    :loop_(loop)
{
    router_ = new LocalMapRepoChannel();
}

TcpClient::~TcpClient()
{
    delete router_;
}

void TcpClient::Close()
{
    for (auto& connector_it : connector_) {
        connector_it->Close();
        delete connector_it;
    }

    connector_.Clear();
    router_->Close();

    uv_stop(mutable_loop());
}

int32_t TcpClient::Connect(const char* host, int32_t port)
{
    Connector* connector = new Connector(mutable_loop(), router_);
    connector_.Add(connector);
    return connector->Connect(host, port);
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
    for (auto& connector_it : connector_) {
        if (connector_it->channel() != maid::Channel::default_instance()) {
            return connector_it->channel();
        }
    }

    return maid::Channel::default_instance();
}

}
