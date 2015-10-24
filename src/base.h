#pragma once

#include <uv.h>
#include "maid/controller.pb.h"
#include "channel_factory.h"
#include "channel.h"

namespace maid {

class TcpServer
{
public:
    TcpServer();
    virtual ~TcpServer();
    virtual void Close();
    void Update();
    void ServeForever();

    int32_t Listen(const std::string& host, int32_t port);

    uv_loop_t* mutable_loop()
    {
        return loop_;
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


private:
    static void OnGC(uv_async_t* handle);
    static void OnClose(uv_async_t* handle);

private:
    void ConnectedCallback(Acceptor* acceptor, int64_t connection_id);
    void DisconnectedCallback(Acceptor* acceptor, int64_t connection_id);
    void CloseCallback(Acceptor* acceptor);

private:
    uv_loop_t* loop_;
    uv_async_t gc_;
    uv_async_t close_;

    google::protobuf::Map<Acceptor*, Acceptor*> acceptor_;
    google::protobuf::RepeatedField<Acceptor*> acceptor_invalid_;
    LocalMapRepoChannel* router_;

    std::vector<std::function<void(int64_t)> > connected_callbacks_;
    std::vector<std::function<void(int64_t)> > disconnected_callbacks_;

};

class TcpClient
{
public:
    TcpClient();
    virtual ~TcpClient();
    virtual void Close();
    void Update();
    void ServeForever();

    int32_t Connect(const std::string& host, int32_t port);

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


private:
    static void OnGC(uv_async_t* handle);
    static void OnClose(uv_async_t* handle);

private:
    void ConnectedCallback(Connector* connector, int64_t connection_id);
    void DisconnectedCallback(Connector* connector, int64_t connection_id);
    void CloseCallback(Connector* connector);

private:
    uv_loop_t* loop_;
    uv_async_t gc_;
    uv_async_t close_;

    google::protobuf::Map<Connector*, int64_t> connector_;
    google::protobuf::RepeatedField<Connector*> connector_invalid_;
    LocalMapRepoChannel* router_;

    std::vector<std::function<void(int64_t)> > connected_callbacks_;
    std::vector<std::function<void(int64_t)> > disconnected_callbacks_;
};

}
