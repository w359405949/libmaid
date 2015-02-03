#include "log_middleware.h"
#include <glog/logging.h>

namespace maid {

LogMiddleware::LogMiddleware()
{
    google::InitGoogleLogging("");
}

void LogMiddleware::Connected(::google::protobuf::RpcController* controller,
        const ::maid::proto::ConnectionProto* request,
        ::maid::proto::ConnectionProto* response,
        ::google::protobuf::Closure* done)
{
    DLOG(INFO)<< "connected: "<< request->DebugString();
}

void LogMiddleware::Disconnected(::google::protobuf::RpcController* controller,
        const ::maid::proto::ConnectionProto* request,
        ::maid::proto::ConnectionProto* response,
        ::google::protobuf::Closure* done)
{
    DLOG(INFO)<< "disconnected: "<< request->DebugString();
}

void LogMiddleware::ProcessRequest(::google::protobuf::RpcController* controller,
        const ::maid::proto::ControllerProto* request,
        ::maid::proto::ControllerProto* response,
        ::google::protobuf::Closure* done)
{
    DLOG(INFO)<< "process request: "<< request->DebugString();
}

void LogMiddleware::ProcessResponse(::google::protobuf::RpcController* controller,
        const ::maid::proto::ControllerProto* request,
        ::maid::proto::ControllerProto* response,
        ::google::protobuf::Closure* done)
{
    DLOG(INFO)<< "process response: "<< request->DebugString();
}

void LogMiddleware::ProcessRequestStub(::google::protobuf::RpcController* controller,
        const ::maid::proto::ControllerProto* request,
        ::maid::proto::ControllerProto* response,
        ::google::protobuf::Closure* done)
{
    DLOG(INFO)<< "process request stub: "<< request->DebugString();
}

void LogMiddleware::ProcessResponseStub(::google::protobuf::RpcController* controller,
        const ::maid::proto::ControllerProto* request,
        ::maid::proto::ControllerProto* response,
        ::google::protobuf::Closure* done)
{
    DLOG(INFO)<< "process request stub: "<< request->DebugString();
}

}
