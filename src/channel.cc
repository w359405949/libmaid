#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <glog/logging.h>

#include "maid/controller.pb.h"
#include "maid/connection.pb.h"
#include "maid/middleware.pb.h"
#include "channel.h"
#include "channel_factory.h"
#include "define.h"
#include "helper.h"
#include "controller.h"
#include "wire_format.h"
#include "closure.h"
#include "uv_hook.h"

namespace maid {

Channel* Channel::default_instance_ = NULL;

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


TcpChannel::TcpChannel(uv_stream_t* stream, AbstractTcpChannelFactory* abs_factory)
    :stream_(stream),
     factory_(abs_factory),
     transmit_id_(0)
{
    signal(SIGPIPE, SIG_IGN);

    stream->data = this;

    timer_handle_.data = this;
    uv_timer_init(maid_default_loop(), &timer_handle_);

    idle_handle_.data = this;
    uv_idle_init(maid_default_loop(), &idle_handle_);

    uv_read_start(stream, OnAlloc, OnRead);
}


AbstractTcpChannelFactory* TcpChannel::factory()
{
    return factory_ == NULL ? AbstractTcpChannelFactory::default_instance() : factory_;
}


TcpChannel::~TcpChannel()
{
    buffer_.reset();
    delete stream_;
}

void TcpChannel::RemoveController(Controller* controller)
{
    router_controllers_[controller] = NULL;
    router_controllers_.erase(controller);
}

void TcpChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* rpc_controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done)
{
    if (stream_ == NULL) {
        rpc_controller->SetFailed("disconnected");
        done->Run();
        return;
    }

    maid::Controller* controller = google::protobuf::down_cast<Controller*>(rpc_controller);
    proto::ControllerProto* controller_proto = controller->mutable_proto();

    controller_proto->set_full_service_name(method->service()->full_name());
    controller_proto->set_method_name(method->name());
    controller_proto->set_stub(true);
    request->SerializeToString(controller_proto->mutable_message());

    int64_t transmit_id = transmit_id_++; // TODO: check if used
    CHECK(async_result_.find(transmit_id) == async_result_.end());
    controller_proto->set_transmit_id(transmit_id);
    async_result_[transmit_id].controller = controller;
    async_result_[transmit_id].response = response;
    async_result_[transmit_id].done = done;

    std::string* send_buffer = WireFormat::Serializer(*controller_proto);
    if (NULL == send_buffer) {
        LOG(ERROR) << " no more memory";
        controller_proto->set_failed(true);
        HandleResponse(controller_proto);
        return;
    }

    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    if (NULL == req) {
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
    self->sending_buffer_[req] = NULL;
    self->sending_buffer_.erase(req);
    free(req);

    if (status != 0) {
        controller_proto->clear_message();
        controller_proto->set_failed(true);
        controller_proto->set_error_text(uv_strerror(status));
        self->HandleResponse(controller_proto);
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

    uv_timer_start(&self->timer_handle_, OnTimer, 1, 1000);
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
        case ERROR_OUT_OF_SIZE:
            break;
        case ERROR_LACK_DATA:
            uv_timer_stop(timer);
            uv_read_start(self->stream_, OnAlloc, OnRead);
            break;
        case ERROR_BUSY:
            break;
        case ERROR_PARSE_FAILED:
            break;
        case ERROR_OTHER:
            break;
        default:
            uv_timer_again(timer);
            break;
    }
}

int32_t TcpChannel::Handle()
{
    proto::ControllerProto* controller_proto = NULL;
    int32_t result = WireFormat::Deserializer(buffer_, &controller_proto);
    if (NULL == controller_proto) {
        return result;
    }

    proto::ConnectionProto* connection = controller_proto->MutableExtension(proto::connection);
    connection->set_id((int64_t)this);

    if (controller_proto->stub()) {
        return HandleRequest(controller_proto);
    } else {
        return HandleResponse(controller_proto);
    }

    return 0;
}

int32_t TcpChannel::HandleRequest(proto::ControllerProto* controller_proto)
{
    const google::protobuf::ServiceDescriptor* service = google::protobuf::DescriptorPool::generated_pool()->FindServiceByName(controller_proto->full_service_name());
    if (NULL == service) {
        DLOG(WARNING) << " service: " << controller_proto->full_service_name() << " not exist";
        delete controller_proto;
        return ERROR_OTHER;
    }

    const google::protobuf::MethodDescriptor* method = NULL;
    method = service->FindMethodByName(controller_proto->method_name());
    if (NULL == method) {
        DLOG(WARNING) << " service: " << controller_proto->full_service_name() << " method: " << controller_proto->method_name() << " not exist";
        delete controller_proto;
        return ERROR_OTHER;
    }

    Controller* controller = NULL;
    google::protobuf::Message* request = NULL;
    google::protobuf::Message* response = NULL;
    TcpClosure* done = NULL;

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

    factory()->router_channel()->CallMethod(method, controller, request, response, done);

    return 0;
}

int32_t TcpChannel::HandleResponse(proto::ControllerProto* controller_proto)
{
    std::map<int64_t, Context>::iterator it = async_result_.find(controller_proto->transmit_id());
    if (it == async_result_.end()) {
        return 0;
    }

    Context& context = it->second;
    if (!controller_proto->failed() && controller_proto->has_message()) {
        context.response->ParseFromString(controller_proto->message());
    }
    context.controller->set_allocated_proto(controller_proto);
    context.done->Run();
    async_result_.erase(controller_proto->transmit_id());

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
    delete handle;
}

void TcpChannel::Close()
{
    uv_idle_stop(&idle_handle_);
    uv_timer_stop(&timer_handle_);
    uv_read_stop(stream_);
    uv_close((uv_handle_t*)stream_, OnCloseStream);
    stream_ = NULL;

    std::map<Controller*, Controller*>::iterator it;
    for (it = router_controllers_.begin(); it != router_controllers_.end(); it++) {
        it->first->StartCancel();
    }
    router_controllers_.clear();

    std::map<int64_t, Context>::iterator context_it;
    for (context_it = async_result_.begin(); context_it != async_result_.end(); context_it++) {
        context_it->second.controller->StartCancel();
        context_it->second.done->Run();
    }

    async_result_.clear();

}


/*
 *
 *
 */

LocalMapRepoChannel::LocalMapRepoChannel()
{
}

LocalMapRepoChannel::~LocalMapRepoChannel()
{
}

void LocalMapRepoChannel::Close()
{
    std::map<const google::protobuf::ServiceDescriptor*, google::protobuf::Service*>::iterator service_it;
    for (service_it = service_.begin(); service_it != service_.end(); service_it++) {
        delete service_it->second;
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
    std::map<const google::protobuf::ServiceDescriptor*, google::protobuf::Service*>::iterator service_it;
    service_it = service_.find(method->service());
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
    std::vector<google::protobuf::Service*>::iterator service_it;
    for (service_it = service_.begin(); service_it != service_.end(); service_it++) {
        delete (*service_it);
    }

    service_.clear();
}

void LocalListRepoChannel::Append(google::protobuf::Service* service)
{
    service_.push_back(service);
}

void LocalListRepoChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done)
{
    std::vector<google::protobuf::Service*>::iterator service_it;
    for (service_it = service_.begin(); service_it != service_.end(); service_it++) {
        (*service_it)->CallMethod(method, controller, request, response, done);
    }
};

} // maid
