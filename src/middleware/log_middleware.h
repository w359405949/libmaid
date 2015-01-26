#pragma once
#include "maid/middleware.pb.h"

class LogMiddleware : public maid::proto::Middleware
{
public:

    void Connected(::google::protobuf::RpcController* controller,
            const ::maid::proto::ConnectionProto* request,
            ::maid::proto::ConnectionProto* response,
            ::google::protobuf::Closure* done);
    void Disconnected(::google::protobuf::RpcController* controller,
            const ::maid::proto::ConnectionProto* request,
            ::maid::proto::ConnectionProto* response,
            ::google::protobuf::Closure* done);
    void ProcessRequest(::google::protobuf::RpcController* controller,
            const ::maid::proto::ControllerProto* request,
            ::maid::proto::ControllerProto* response,
            ::google::protobuf::Closure* done);
    void ProcessResponse(::google::protobuf::RpcController* controller,
            const ::maid::proto::ControllerProto* request,
            ::maid::proto::ControllerProto* response,
            ::google::protobuf::Closure* done);
    void ProcessRequestStub(::google::protobuf::RpcController* controller,
            const ::maid::proto::ControllerProto* request,
            ::maid::proto::ControllerProto* response,
            ::google::protobuf::Closure* done);
    void ProcessResponseStub(::google::protobuf::RpcController* controller,
            const ::maid::proto::ControllerProto* request,
            ::maid::proto::ControllerProto* response,
            ::google::protobuf::Closure* done);

    LogMiddleware();
};
