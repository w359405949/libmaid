#pragma once

#include <uv.h>
#include <vector>
#include "maid/controller.pb.h"
#include "maid/middleware.pb.h"
#include "channel_factory.h"
#include "channel_pool.h"
#include "channel.h"

namespace maid {

class TcpServer
{
public:
    TcpServer();
    ~TcpServer();

    int32_t Listen(const char* host, int32_t port, int32_t backlog=1);

    void ServeForever();
    void Update();

    void InsertService(google::protobuf::Service* service);
    void AppendMiddleware(maid::proto::Middleware* middleware);

    ChannelPool* pool() const;

    void Close();

private:
    std::vector<Acceptor*> acceptor_;
    LocalMapRepoChannel* router_;
    LocalListRepoChannel* middleware_;
    ChannelPool* pool_;
};

class TcpClient
{
public:
    TcpClient();
    ~TcpClient();

    int32_t Connect(const char* host, int32_t port);

    void Update();
    void ServeForever();

    void InsertService(google::protobuf::Service* service);
    void AppendMiddleware(maid::proto::Middleware* middleware);

    google::protobuf::RpcChannel* channel() const;

    void Close();

private:
    std::vector<Connector*> connector_;

    LocalMapRepoChannel* router_;
    LocalListRepoChannel* middleware_;
    ChannelPool* pool_;
};

}
