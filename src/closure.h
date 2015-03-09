#pragma once
#include <stdio.h>
#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <uv.h>

namespace maid {
class Controller;
class TcpChannel;

class Closure : public google::protobuf::Closure
{
public:
    Closure();
    virtual ~Closure();

    void Run();

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Closure);
};

class GCClosure : public google::protobuf::Closure
{
public:
    GCClosure(google::protobuf::RpcController* controller,
            google::protobuf::Message* request,
            google::protobuf::Message* response)
        :controller_(controller),
        request_(request),
        response_(response),
        called(false)
    {
    }

    virtual ~GCClosure()
    {
        delete controller_;
        delete request_;
        delete response_;
    }

    void Run();

public:
    static void OnGC(uv_idle_t* handle);

private:
    google::protobuf::RpcController* controller_;
    google::protobuf::Message* request_;
    google::protobuf::Message* response_;
    bool called;
    uv_idle_t gc_;
};

class TcpClosure : public google::protobuf::Closure
{
public:
    TcpClosure(TcpChannel* channel, Controller* controller, google::protobuf::Message* request, google::protobuf::Message* response);
    virtual ~TcpClosure();

    void Run();

public:
    static void AfterSendResponse(uv_write_t* handle, int32_t status);
    static void OnGc(uv_idle_t* idle);

public: // unit test only
    inline const TcpChannel* channel() const
    {
        return channel_;
    }

    inline const std::string* send_buffer() const
    {
        return send_buffer_;
    }

    inline const Controller* controller() const
    {
        return controller_;
    }

    inline const google::protobuf::Message* request() const
    {
        return request_;
    }

    inline const google::protobuf::Message* response() const
    {
        return response_;
    }

    inline const uv_write_t& req() const
    {
        return req_;
    }

    inline const uv_idle_t& gc() const
    {
        return gc_;
    }

private:
    TcpChannel* channel_;
    std::string* send_buffer_;
    Controller* controller_;
    google::protobuf::Message* request_;
    google::protobuf::Message* response_;

    uv_write_t req_;
    uv_idle_t gc_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(TcpClosure);
};

}
