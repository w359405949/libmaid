#pragma once

#include <uv.h>
#include "maid/controller.pb.h"
#include "channel_factory.h"
#include "channel.h"

namespace maid {

class TcpServer
{
public:
    TcpServer(uv_loop_t* loop=NULL);
    virtual ~TcpServer();

    int32_t Listen(const std::string& host, int32_t port, int32_t backlog=1);

    void ServeForever();
    void Update();
    uv_loop_t* mutable_loop()
    {
        if (loop_ != NULL) {
            return loop_;
        }
        return uv_default_loop();
    }

    google::protobuf::RpcChannel* channel(int64_t channel_id);

    void InsertService(google::protobuf::Service* service);
    inline void AddConnectedCallback(std::function<void(int64_t)> callback)
    {
        connected_callbacks_.push_back(callback);
    }

    inline void AddDisconnectedCallback(std::function<void(int64_t)> callback)
    {
        disconnected_callbacks_.push_back(callback);
    }

    void Close();

private:
    void ConnectedCallback(int32_t index, int64_t connection_id);
    void DisconnectedCallback(int32_t index, int64_t connection_id);

private:
    uv_loop_t* loop_;
    int32_t current_index_;
    google::protobuf::Map<int32_t, Acceptor*> acceptor_;
    LocalMapRepoChannel* router_;

    std::vector<std::function<void(int64_t)> > connected_callbacks_;
    std::vector<std::function<void(int64_t)> > disconnected_callbacks_;

};

class TcpClient
{
public:
    TcpClient(uv_loop_t* loop=NULL);
    virtual ~TcpClient();

    int32_t Connect(const std::string& host, int32_t port);

    void Update();
    void ServeForever();
    uv_loop_t* mutable_loop()
    {
        if (loop_ != NULL) {
            return loop_;
        }
        return uv_default_loop();
    }

    void InsertService(google::protobuf::Service* service);

    inline void AddConnectedCallback(std::function<void(int64_t)> callback)
    {
        connected_callbacks_.push_back(callback);
    }

    inline void AddDisconnectedCallback(std::function<void(int64_t)> callback)
    {
        disconnected_callbacks_.push_back(callback);
    }

    google::protobuf::RpcChannel* channel() const;

    void Close();

private:
    void ConnectedCallback(int32_t index, int64_t connection_id);
    void DisconnectedCallback(int32_t index, int64_t connection_id);

private:
    uv_loop_t* loop_;
    int32_t current_index_;
    google::protobuf::Map<int32_t, Connector*> connector_;
    LocalMapRepoChannel* router_;


    std::vector<std::function<void(int64_t)> > connected_callbacks_;
    std::vector<std::function<void(int64_t)> > disconnected_callbacks_;
};

}
