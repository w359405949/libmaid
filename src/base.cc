#include "base.h"
#include "uv_hook.h"

namespace maid {

TcpServer::TcpServer()
{
    router_ = new LocalMapRepoChannel();
    middleware_ = new LocalListRepoChannel();
    pool_ = new ChannelPool();
    pool_->AddChannel(router_);
    pool_->AddChannel(middleware_);
}

TcpServer::~TcpServer()
{
    delete router_;
    delete middleware_;
    delete pool_;
}

void TcpServer::Close()
{
    std::vector<Acceptor*>::iterator it;
    for (it = acceptor_.begin(); it == acceptor_.end(); it++) {
        (*it)->Close();
        delete (*it);
    }
    acceptor_.clear();

    router_->Close();
    middleware_->Close();
    pool_->Close();

    uv_stop(maid_default_loop());
}

int32_t TcpServer::Listen(const char* host, int32_t port, int32_t backlog)
{
    Acceptor* acceptor = new Acceptor(router_, middleware_, pool_);
    acceptor_.push_back(acceptor);
    return acceptor->Listen(host, port, backlog);
}

void TcpServer::ServeForever()
{
    uv_run(maid_default_loop(), UV_RUN_DEFAULT);
}

void TcpServer::Update()
{
    uv_run(maid_default_loop(), UV_RUN_NOWAIT);
}

void TcpServer::InsertService(google::protobuf::Service* service)
{
    router_->Insert(service);
}

void TcpServer::AppendMiddleware(proto::Middleware* middleware)
{
    middleware_->Append(middleware);
}

ChannelPool* TcpServer::pool() const
{
    return pool_;
}


/*
 *
 *
 *
 */

TcpClient::TcpClient()
{
    router_ = new LocalMapRepoChannel();
    middleware_ = new LocalListRepoChannel();
    pool_ = new ChannelPool();
    pool_->AddChannel(router_);
    pool_->AddChannel(middleware_);
}

TcpClient::~TcpClient()
{
    delete router_;
    delete middleware_;
    delete pool_;
}

void TcpClient::Close()
{
    std::vector<Connector*>::const_iterator it;
    for (it = connector_.begin(); it != connector_.end(); it++) {
        (*it)->Close();
        delete (*it);
    }
    connector_.clear();
    router_->Close();
    middleware_->Close();
    pool_->Close();

    uv_stop(maid_default_loop());
}

int32_t TcpClient::Connect(const char* host, int32_t port)
{
    Connector* connector = new Connector(router_, middleware_, pool_);
    connector_.push_back(connector);
    return connector->Connect(host, port);
}

void TcpClient::InsertService(google::protobuf::Service* service)
{
    router_->Insert(service);
}

void TcpClient::AppendMiddleware(proto::Middleware* middleware)
{
    middleware_->Append(middleware);
}

void TcpClient::ServeForever()
{
    uv_run(maid_default_loop(), UV_RUN_DEFAULT);
}

void TcpClient::Update()
{
    uv_run(maid_default_loop(), UV_RUN_NOWAIT);
}

google::protobuf::RpcChannel* TcpClient::channel() const
{
    std::vector<Connector*>::const_iterator it;
    for (it = connector_.begin(); it != connector_.end(); it++) {
        if ((*it)->channel() != maid::Channel::default_instance()) {
            return (*it)->channel();
        }
    }

    return maid::Channel::default_instance();
}

}
