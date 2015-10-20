#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/empty.pb.h>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/wrappers.pb.h>
#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/stl_util.h>

#include "maid/controller.pb.h"
#include "channel.h"
#include "channel_factory.h"
#include "controller.h"

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


TcpChannel::TcpChannel(uv_stream_t* stream, AbstractTcpChannelFactory* abs_factory)
     :stream_(stream),
     buffer_length_(0),
     factory_(abs_factory)
{
    signal(SIGPIPE, SIG_IGN);

    stream_->data = this;
    uv_tcp_nodelay((uv_tcp_t*)stream_, 1);
    uv_read_start(stream_, OnAlloc, OnRead);

    write_handle_ = (uv_async_t*)malloc(sizeof(uv_async_t));
    write_handle_->data = this;
    uv_async_init(stream_->loop, write_handle_, OnWrite);

    proto_handle_ = (uv_async_t*)malloc(sizeof(uv_async_t));
    proto_handle_->data = this;
    uv_async_init(stream_->loop, proto_handle_, OnHandle);

    uv_mutex_init(&write_buffer_mutex_);
    uv_mutex_init(&read_proto_mutex_);
}

TcpChannel::~TcpChannel()
{
    GOOGLE_CHECK(reading_buffer_.empty());
    GOOGLE_CHECK(queue_closure_.empty());
    GOOGLE_CHECK(result_callback_.empty());

    stream_ = nullptr;
    factory_ = nullptr;

    uv_mutex_destroy(&write_buffer_mutex_);
    uv_mutex_destroy(&read_proto_mutex_);
}

void TcpChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* _controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done)
{
    Controller* controller = (Controller*)_controller;
    auto* controller_proto = controller->mutable_proto();

    controller_proto->set_full_service_name(method->service()->full_name());
    controller_proto->set_method_name(method->name());
    controller_proto->set_stub(true);
    controller_proto->set_transmit_id((int64_t)controller_proto);
    controller_proto->mutable_message()->PackFrom(*request);

    if (response->GetDescriptor() != google::protobuf::Empty::descriptor()) {
        result_callback_[controller_proto->transmit_id()] = google::protobuf::NewPermanentCallback(this, &TcpChannel::ResultCall, method, controller, request, response, done);
    } else {
        queue_closure_.Add(done);
    }

    uv_mutex_lock(&write_buffer_mutex_);
    {
        google::protobuf::io::StringOutputStream string_stream(&write_buffer_);
        google::protobuf::io::CodedOutputStream coded_stream(&string_stream);
        coded_stream.WriteVarint32(controller_proto->ByteSize());
        controller_proto->SerializeToCodedStream(&coded_stream);
    }
    if (write_handle_ != nullptr) {
        uv_async_send(write_handle_);
    }
    uv_mutex_unlock(&write_buffer_mutex_);
}


void* TcpChannel::ResultCall(const google::protobuf::MethodDescriptor* method, Controller* controller, const google::protobuf::Message* request, google::protobuf::Message* response, google::protobuf::Closure* done, const proto::ControllerProto& controller_proto, void*)
{
    controller->mutable_proto()->MergeFrom(controller_proto);
    controller->proto().message().UnpackTo(response);

    queue_closure_.Add(done);

    return nullptr;
}

void TcpChannel::OnWrite(uv_async_t* handle)
{
    if (uv_is_closing((uv_handle_t*)handle)) {
        return;
    }

    TcpChannel* self = (TcpChannel*)handle->data;

    uv_mutex_lock(&self->write_buffer_mutex_);
    {
        std::swap(self->write_buffer_, self->write_buffer_back_);
    }
    uv_mutex_unlock(&self->write_buffer_mutex_);

    if (self->write_buffer_back_.empty()) {
        return;
    }

    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    req->data = self;

    uv_buf_t buf;
    buf.base = (char*)self->write_buffer_back_.data();
    buf.len = self->write_buffer_back_.size();
    int error = uv_write(req, self->stream_, &buf, 1, AfterWrite);
    if (error) {
        free(req);
    }
}

void TcpChannel::AfterWrite(uv_write_t* req, int32_t status)
{
    TcpChannel* self = (TcpChannel*)req->data;
    free(req);

    self->write_buffer_back_.clear();

    GOOGLE_LOG_IF(WARNING, status != 0) << uv_strerror(status);
    if (status < 0) {
        //self->factory_->QueueChannelInvalid(self);
        return; //
    }
}


void TcpChannel::OnAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    TcpChannel* self = (TcpChannel*)handle->data;

    std::string* buffer = self->read_stream_.ReleaseCleared();
    GOOGLE_CHECK(buffer->empty());

    google::protobuf::STLStringResizeUninitialized(buffer, suggested_size);

    buf->base = (char*)buffer->data();
    buf->len = buffer->capacity();

    self->reading_buffer_[buf->base] = buffer;
}


void TcpChannel::OnRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    TcpChannel* self = (TcpChannel*)stream->data;

    GOOGLE_CHECK(self->reading_buffer_.find(buf->base) != self->reading_buffer_.end());//TODO: buf->base alloced by libuv (win).

    std::string* buffer = self->reading_buffer_[buf->base];
    self->reading_buffer_.erase(buf->base);

    if (nread >= 0) {
        GOOGLE_CHECK(buffer->capacity() >= nread) << "overflowed";

        google::protobuf::STLStringResizeUninitialized(buffer, nread);
        self->read_stream_.AddBuffer(buffer);
        self->buffer_length_ += nread;

        uv_async_send(self->proto_handle_);
    } else {
        self->read_stream_.AddBuffer(buffer); // recircle
        self->factory_->QueueChannelInvalid(self);
    }
}

void TcpChannel::OnHandle(uv_async_t* handle)
{
    if (uv_is_closing((uv_handle_t*)handle)) {
        return;
    }

    TcpChannel* self = (TcpChannel*)handle->data;

    google::protobuf::io::CodedInputStream coded_stream(&self->read_stream_);
    coded_stream.SetTotalBytesLimit(self->buffer_length_, self->buffer_length_);

    // TODO: message trunc
    while (coded_stream.BytesUntilTotalBytesLimit() >= 10) { // kMaxVarintBytes
        uint32_t length;
        if (!coded_stream.ReadVarint32(&length)) {
            break;
        }

        if (length > coded_stream.BytesUntilTotalBytesLimit()) {
            break;
        }

        if (length == 0) {
            break;
        }

        auto limit = coded_stream.PushLimit(length);
        uv_mutex_lock(&self->read_proto_mutex_);
        {
            self->read_proto_.Add()->ParseFromCodedStream(&coded_stream);
        }
        uv_mutex_unlock(&self->read_proto_mutex_);
        coded_stream.PopLimit(limit);

        self->buffer_length_ = coded_stream.BytesUntilTotalBytesLimit();
    }
}

void TcpChannel::Update()
{
    google::protobuf::RepeatedPtrField<proto::ControllerProto> read_proto_back;
    uv_mutex_lock(&read_proto_mutex_);
    {
        read_proto_back.Swap(&read_proto_);
    }
    uv_mutex_unlock(&read_proto_mutex_);

    for (auto& controller_proto : read_proto_back) {
        controller_proto.set_connection_id((int64_t)this);
        if (controller_proto.stub()) {
            HandleRequest(controller_proto);
        } else {
            GOOGLE_DCHECK(result_callback_.find(controller_proto.transmit_id()) != result_callback_.end());
            result_callback_[controller_proto.transmit_id()]->Run(controller_proto, NULL);
            result_callback_.erase(controller_proto.transmit_id());
        }
    }

    for (auto& closure : queue_closure_) {
        closure->Run();
    }
    queue_closure_.Clear();
}

int32_t TcpChannel::HandleRequest(const proto::ControllerProto& controller_proto)
{
    const auto* service = google::protobuf::DescriptorPool::generated_pool()->FindServiceByName(controller_proto.full_service_name());
    GOOGLE_LOG_IF(WARNING, service == nullptr) << " service: " << controller_proto.full_service_name() << " not exist";
    if (nullptr == service) {
        return -1;
    }

    const auto* method = service->FindMethodByName(controller_proto.method_name());
    GOOGLE_LOG_IF(WARNING, method == nullptr) << " service: " << controller_proto.full_service_name() << " method: " << controller_proto.method_name() << " not exist";
    if (nullptr == method) {
        return -1;
    }

    Controller* controller = new Controller();
    auto* request = google::protobuf::MessageFactory::generated_factory()->GetPrototype(method->input_type())->New();
    auto* response = google::protobuf::MessageFactory::generated_factory()->GetPrototype(method->output_type())->New();
    auto* done = closure_component_.NewClosure(&TcpChannel::AfterHandleRequest, this, controller, request, response);

    controller->mutable_proto()->MergeFrom(controller_proto);
    controller->proto().message().UnpackTo(request);

    factory_->router_channel()->CallMethod(method, controller, request, response, done);

    return 0;
}

void TcpChannel::AfterHandleRequest(google::protobuf::RpcController* controller, google::protobuf::Message* request, google::protobuf::Message* response)
{
    if (response->GetDescriptor() == google::protobuf::Empty::descriptor()) {
        return;
    }

    proto::ControllerProto* controller_proto = ((Controller*)controller)->mutable_proto();
    controller_proto->set_stub(false);
    controller_proto->mutable_message()->PackFrom(*response);

    uv_mutex_lock(&write_buffer_mutex_);
    {
        google::protobuf::io::StringOutputStream string_stream(&write_buffer_);
        google::protobuf::io::CodedOutputStream coded_stream(&string_stream);
        coded_stream.WriteVarint32(controller_proto->ByteSize());
        controller_proto->SerializeToCodedStream(&coded_stream);
    }
    if (write_handle_ != nullptr) {
        uv_async_send(write_handle_);
    }
    uv_mutex_unlock(&write_buffer_mutex_);
}

void TcpChannel::OnClose(uv_handle_t* handle)
{
    free(handle);
}

void TcpChannel::Close()
{
    uv_close((uv_handle_t*)stream_, OnClose);
    uv_close((uv_handle_t*)proto_handle_, OnClose);

    uv_mutex_lock(&write_buffer_mutex_);
    {
        if (write_handle_ != nullptr) {
            uv_close((uv_handle_t*)write_handle_, OnClose);
            write_handle_ = nullptr;
        }
    }
    uv_mutex_unlock(&write_buffer_mutex_);
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
    GOOGLE_CHECK(service_.find(service->GetDescriptor()) == service_.end());

    service_[service->GetDescriptor()] = service;
}

void LocalMapRepoChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done)
{
    auto service_it = service_.find(method->service());
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
