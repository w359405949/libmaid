#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/empty.pb.h>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/wrappers.pb.h>
#include <glog/logging.h>

#include "maid/controller.pb.h"
#include "channel.h"
#include "channel_factory.h"
#include "define.h"
#include "controller.h"
#include "wire_format.h"
#include "closure.h"

namespace maid {

Channel* Channel::default_instance_ = nullptr;

void InitChannel()
{
    Channel::default_instance_ = new Channel();
}

struct StaticInitChannel
{
    StaticInitChannel()
    {
        InitChannel(); // force call init channel;
    }

} static_init_channel;

Channel::Channel()
{
}

void Channel::CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done)
{
    controller->SetFailed("nothing to be done");
    done->Run();
}


TcpChannel::TcpChannel(uv_loop_t* loop, uv_stream_t* stream, AbstractTcpChannelFactory* abs_factory)
    :loop_(loop),
     stream_(stream),
     factory_(abs_factory),
     transmit_id_(0)
{
    signal(SIGPIPE, SIG_IGN);

    stream->data = this;

    timer_handle_.data = this;
    uv_timer_init(loop_, &timer_handle_);

    idle_handle_.data = this;
    uv_idle_init(loop_, &idle_handle_);

    uv_read_start(stream, OnAlloc, OnRead);
}


AbstractTcpChannelFactory* TcpChannel::factory()
{
    return factory_;
}


TcpChannel::~TcpChannel()
{
    CHECK(stream_ == nullptr)<<"call Close first";
}

void TcpChannel::RemoveController(Controller* controller)
{
    router_controllers_[controller] = nullptr;
    router_controllers_.erase(controller);
}

void TcpChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* rpc_controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done)
{
    if (stream_ == nullptr) {
        rpc_controller->SetFailed("disconnected");
        done->Run();
        return;
    }

    proto::ControllerProto* controller_proto = new proto::ControllerProto();

    controller_proto->set_full_service_name(method->service()->full_name());
    controller_proto->set_method_name(method->name());
    controller_proto->set_stub(true);
    request->SerializeToString(controller_proto->mutable_message());

    // delay callback
    if (response->GetDescriptor() != google::protobuf::Empty::descriptor()) {
        int64_t transmit_id = transmit_id_++; // TODO: check if used
        CHECK(async_result_.find(transmit_id) == async_result_.end());
        controller_proto->set_transmit_id(transmit_id);
        async_result_[transmit_id].controller = google::protobuf::down_cast<Controller*>(rpc_controller);
        async_result_[transmit_id].response = response;
        async_result_[transmit_id].done = done;
    } else {
        done->Run();
    }

    std::string* send_buffer = WireFormat::Serializer(*controller_proto);
    if (nullptr == send_buffer) {
        LOG(ERROR) << " no more memory";
        controller_proto->set_failed(true);
        HandleResponse(controller_proto);
        return;
    }

    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    if (nullptr == req) {
        delete send_buffer;
        LOG(ERROR) << " no more memory";
        controller_proto->set_failed(true);
        HandleResponse(controller_proto);
        return;
    }
    req->data = controller_proto;

    uv_buf_t uv_buf;
    uv_buf.base = (char*)send_buffer->data();
    uv_buf.len = send_buffer->size();
    int error = uv_write(req, stream_, &uv_buf, 1, AfterSendRequest);
    if (error) {
        free(req);
        delete send_buffer;
        DLOG(ERROR) << uv_strerror(error);
        controller_proto->set_failed(true);
        controller_proto->set_error_text(uv_strerror(error));
        HandleResponse(controller_proto);
        return;
    }

    sending_buffer_[req] = send_buffer;


}

void TcpChannel::AfterSendRequest(uv_write_t* req, int32_t status)
{
    TcpChannel* self = (TcpChannel*)req->handle->data;
    proto::ControllerProto* controller_proto = (proto::ControllerProto*)req->data;
    delete self->sending_buffer_[req];
    self->sending_buffer_.erase(req);
    free(req);

    if (status != 0) {
        controller_proto->set_failed(true);
        controller_proto->set_error_text(uv_strerror(status));
        controller_proto->clear_message();
        self->HandleResponse(controller_proto);
    } else {
        delete controller_proto;
    }
}

void TcpChannel::OnRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    uv_read_stop(stream);

    TcpChannel* self = (TcpChannel*)stream->data;
    if (nread < 0) {
        self->factory()->Disconnected(self);
        return;
    }
    /*
     * TODO: buf->base alloced by libuv (win).
     */
    self->buffer_.end += nread;

    uv_timer_start(&self->timer_handle_, OnTimer, 1, 1);
}

void TcpChannel::OnIdle(uv_idle_t* idle)
{
    TcpChannel* self = (TcpChannel*)idle->data;
    int32_t result = self->Handle();

    // scheduler
    switch (result) {
        case ERROR_OUT_OF_SIZE:
            break;
        case ERROR_LACK_DATA:
            uv_idle_stop(idle);
            uv_read_start(self->stream_, OnAlloc, OnRead);
            break;
        case ERROR_BUSY:
            break;
        case ERROR_PARSE_FAILED:
            break;
        case ERROR_OTHER:
            break;
        default:
            break;
    }
}

void TcpChannel::OnTimer(uv_timer_t* timer)
{
    TcpChannel* self = (TcpChannel*)timer->data;
    int32_t result = self->Handle();

    // scheduler
    switch (result) {
        case ERROR_LACK_DATA:
            uv_timer_stop(timer);
            uv_read_start(self->stream_, OnAlloc, OnRead);
            break;
        case ERROR_OUT_OF_SIZE:
        case ERROR_BUSY:
        case ERROR_PARSE_FAILED:
        case ERROR_OTHER:
        default:
            uv_timer_again(timer);
            break;
    }
}

int32_t TcpChannel::Handle()
{
    proto::ControllerProto* controller_proto = nullptr;
    int32_t result = WireFormat::Deserializer(buffer_, &controller_proto);
    if (nullptr == controller_proto) {
        return result;
    }

    if (controller_proto->stub()) {
        return HandleRequest(controller_proto);
    } else {
        return HandleResponse(controller_proto);
    }

    return 0;
}

int32_t TcpChannel::HandleRequest(proto::ControllerProto* controller_proto)
{
    const auto* service = google::protobuf::DescriptorPool::generated_pool()->FindServiceByName(controller_proto->full_service_name());
    if (nullptr == service) {
        DLOG(WARNING) << " service: " << controller_proto->full_service_name() << " not exist";
        delete controller_proto;
        return ERROR_OTHER;
    }

    const auto* method = service->FindMethodByName(controller_proto->method_name());
    if (nullptr == method) {
        DLOG(WARNING) << " service: " << controller_proto->full_service_name() << " method: " << controller_proto->method_name() << " not exist";
        delete controller_proto;
        return ERROR_OTHER;
    }

    Controller* controller = nullptr;
    google::protobuf::Message* request = nullptr;
    google::protobuf::Message* response = nullptr;
    TcpClosure* done = nullptr;

    try {
        controller = new Controller();
        controller->set_allocated_proto(controller_proto);

        request = google::protobuf::MessageFactory::generated_factory()->GetPrototype(method->input_type())->New();
        if (!request->ParseFromString(controller_proto->message())) {
            delete controller;
            delete request;
            delete done;
            return ERROR_PARSE_FAILED;
        }

        response = google::protobuf::MessageFactory::generated_factory()->GetPrototype(method->output_type())->New();

        done = new TcpClosure(this, controller, request, response);

        router_controllers_[controller] = controller;
    } catch (std::bad_alloc) {
        delete controller;
        delete request;
        delete response;
        delete done;
        return ERROR_BUSY;
    }

    controller_proto->set_connection_id((int64_t)this);
    factory()->router_channel()->CallMethod(method, controller, request, response, done);

    return 0;
}

int32_t TcpChannel::HandleResponse(proto::ControllerProto* controller_proto)
{
    const auto& async_result_it = async_result_.find(controller_proto->transmit_id());
    if (async_result_it == async_result_.end()) {
        delete controller_proto;
        return 0;
    }

    Controller* controller = async_result_it->second.controller;
    google::protobuf::Closure* done = async_result_it->second.done;
    google::protobuf::Message* response = async_result_it->second.response;

    async_result_it->second.reset();
    async_result_.erase(async_result_it);

    if (!controller_proto->failed() && controller_proto->message().size() > 0) {
        response->ParseFromString(controller_proto->message());
    }
    controller->set_allocated_proto(controller_proto);
    done->Run();
    return 0;
}

void TcpChannel::OnAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    TcpChannel* self = (TcpChannel*)handle->data;

    buf->len = self->buffer_.Expend(suggested_size);
    buf->base = (char*)self->buffer_.end;
}

void TcpChannel::OnCloseStream(uv_handle_t* handle)
{
    free(handle);
}

void TcpChannel::Close()
{
    uv_idle_stop(&idle_handle_);
    uv_timer_stop(&timer_handle_);
    uv_read_stop(stream_);
    uv_close((uv_handle_t*)stream_, OnCloseStream);
    stream_ = nullptr;

    for (auto& controller_it : router_controllers_) {
        controller_it.first->StartCancel();
    }
    router_controllers_.clear();

    for (auto& context_it : async_result_) {
        context_it.second.controller->StartCancel();
        context_it.second.controller->SetFailed("canceled");
        context_it.second.done->Run();
        context_it.second.reset();
    }

    async_result_.clear();

    buffer_.reset();
}


LocalMapRepoChannel::LocalMapRepoChannel()
{
}

LocalMapRepoChannel::~LocalMapRepoChannel()
{
}

void LocalMapRepoChannel::Close()
{
    for (auto& service_it : service_) {
        delete service_it.second;
    }
    service_.clear();
}

void LocalMapRepoChannel::Insert(google::protobuf::Service* service)
{
    DCHECK(service_.find(service->GetDescriptor()) == service_.end());

    service_[service->GetDescriptor()] = service;
}

void LocalMapRepoChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done)
{
    const auto& service_it = service_.find(method->service());
    if (service_.end() == service_it) {
        controller->SetFailed("service not implement");
        done->Run();
        return;
    }

    service_it->second->CallMethod(method, controller, request, response, done);
};


LocalListRepoChannel::LocalListRepoChannel()
{
}

LocalListRepoChannel::~LocalListRepoChannel()
{
}

void LocalListRepoChannel::Close()
{
    for (auto& service_it : service_) {
        delete service_it;
    }

    service_.Clear();
}

void LocalListRepoChannel::Append(google::protobuf::Service* service)
{

    service_.Add(service);
}

void LocalListRepoChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done)
{
    for (auto& service_it : service_) {
        service_it->CallMethod(method, controller, request, response, done);
    }
};

} // maid
