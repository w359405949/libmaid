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
    ~TcpServer();

    int32_t Listen(const char* host, int32_t port, int32_t backlog=1);

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

    void AddDisconnectedCallback(std::function<void(int64_t)> callback)
    {
        disconnected_callbacks_.push_back(callback);
    }

    void Close();

private:
    inline void ConnectedCallback(int64_t connection_id)
    {
        for (auto& callback : connected_callbacks_) {
            callback(connection_id);
        }
    }

    inline void DisconnectedCallback(int64_t connection_id)
    {
        for (auto& callback : disconnected_callbacks_) {
            callback(connection_id);
        }
    }

private:
    uv_loop_t* loop_;
    google::protobuf::RepeatedField<Acceptor*> acceptor_;
    LocalMapRepoChannel* router_;

    std::vector<std::function<void(int64_t)> > connected_callbacks_;
    std::vector<std::function<void(int64_t)> > disconnected_callbacks_;

};

class TcpClient
{
public:
    TcpClient(uv_loop_t* loop=NULL);
    ~TcpClient();

    int32_t Connect(const char* host, int32_t port);

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

    void AddDisconnectedCallback(std::function<void(int64_t)> callback)
    {
        disconnected_callbacks_.push_back(callback);
    }

    google::protobuf::RpcChannel* channel() const;

    void Close();

private:
    void ConnectedCallback(int64_t connection_id)
    {
        for (auto& callback : connected_callbacks_) {
            callback(connection_id);
        }
    }

    void DisconnectedCallback(int64_t connection_id)
    {
        for (auto& callback : disconnected_callbacks_) {
            callback(connection_id);
        }
    }

private:
    uv_loop_t* loop_;
    google::protobuf::RepeatedField<Connector*> connector_;
    LocalMapRepoChannel* router_;


    std::vector<std::function<void(int64_t)> > connected_callbacks_;
    std::vector<std::function<void(int64_t)> > disconnected_callbacks_;
};

}
