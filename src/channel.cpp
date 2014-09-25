#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <uv.h>
#include "channel.h"
#include "controller.h"
#include "closure.h"
#include "define.h"

using maid::channel::Channel;
using maid::channel::Buffer;
using maid::controller::Controller;
using maid::proto::ControllerMeta;
using maid::closure::Closure;
using maid::closure::RemoteClosure;

Channel::Channel(uv_loop_t* loop)
    :loop_(loop),
    controller_max_length_(10000),
     default_fd_(0)
{
    ASSERT(NULL != loop, "libmaid: loop can not be none");
    signal(SIGPIPE, SIG_IGN);
    //uv_idle_init(loop, &remote_closure_gc_);
    //uv_idle_start(&remote_closure_gc_, OnRemoteClosureGC);
}

Channel::~Channel()
{
    uv_idle_stop(&remote_closure_gc_);
}

void Channel::AppendService(google::protobuf::Service* service)
{
    ASSERT(NULL != service, "libmaid: service can not be NULL");
    ASSERT(service_.find(service->GetDescriptor()->full_name()) == service_.end(), "same name service regist twice");

    service_[service->GetDescriptor()->full_name()] = service;
}

void Channel::CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* rpc_controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* rpc_done)
{
    ASSERT(dynamic_cast<Controller*>(rpc_controller), "libmaid: controller must have type maid::controller::Controller");
    ASSERT(request, "libmaid: should not be NULL");

    Controller* controller = (Controller*)rpc_controller;
    if (!controller->fd()) {
        controller->set_fd(default_fd_);
    }
    controller->meta_data().set_full_service_name(method->service()->full_name());
    controller->meta_data().set_method_name(method->name());

    if (controller->meta_data().notify()) {
        SendNotify(controller, request);
    } else {
        ASSERT(request && response, "libmaid: should not be NULL");
        ASSERT(dynamic_cast<Closure*>(rpc_done), "libmaid: done must have type maid::closure::Closure");
        Closure* done = (Closure*)rpc_done;
        done->set_request(request);
        done->set_controller(controller);
        done->set_response(response);
        SendRequest(controller, request, done);
    }
}

void Channel::AfterSendNotify(uv_write_t* req, int32_t status)
{
    free(req);
}

void Channel::SendNotify(maid::controller::Controller* controller, const google::protobuf::Message* request)
{
    std::map<int64_t, uv_stream_t*>::const_iterator it = connected_handle_.find(controller->fd());
    if (connected_handle_.end() == it) {
        WARN("not connected");
        return;
    }

    std::string buffer;
    std::string* message = controller->meta_data().mutable_message(); //TODO
    request->SerializeToString(message);
    int32_t controller_nl = ::htonl(controller->meta_data().ByteSize());
    buffer.append((const char*)&controller_nl, sizeof(controller_nl));
    controller->meta_data().AppendToString(&buffer);

    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    req->data = controller;

    uv_buf_t uv_buf;
    uv_buf.base = (char*)buffer.c_str();
    uv_buf.len = buffer.length();
    int error = uv_write(req, it->second, &uv_buf, 1, AfterSendNotify);
    if (error) {
        WARN("%s", uv_strerror(error));
        free(req);
    }
}

void Channel::AfterSendRequest(uv_write_t* req, int32_t status)
{
    if (status) {
        Controller* controller = (Controller*)req->data;
        Channel* self = (Channel*)req->handle->data;
        controller->SetFailed(uv_strerror(status));
        self->HandleResponse(controller);
    }
    free(req);
}

void Channel::SendRequest(Controller* controller, const google::protobuf::Message* request, maid::closure::Closure* done)
{
    controller->meta_data().set_stub(true);
    std::map<int64_t, uv_stream_t*>::const_iterator it = connected_handle_.find(controller->fd());
    if (it == connected_handle_.end()) {
        controller->SetFailed("not connected");
        done->Run();
        return;
    }

    /*
     *  TODO:
     */
    uint64_t transmit_id = async_result_.size() + 1;
    for (uint64_t i = 1; i < transmit_id; i++) {
        if (async_result_.end() == async_result_.find(i)) {
            transmit_id = i;
            break;
        }
    }
    controller->meta_data().set_transmit_id(transmit_id);
    async_result_[transmit_id] = done;
    transactions_[controller->fd()].insert(transmit_id);

    std::string buffer;
    std::string* message = controller->meta_data().mutable_message(); //TODO
    request->SerializeToString(message);
    int32_t controller_nl = ::htonl(controller->meta_data().ByteSize());
    buffer.append((const char*)&controller_nl, sizeof(controller_nl));
    controller->meta_data().AppendToString(&buffer);

    uv_buf_t uv_buf;
    uv_buf.base = (char*)buffer.c_str();
    uv_buf.len = buffer.length();
    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    req->data = controller;
    int error = uv_write(req, it->second, &uv_buf, 1, AfterSendRequest);
    if (error) {
        done->controller()->SetFailed("send failed");
        HandleResponse(controller);
        free(req);
    }
}

void Channel::AfterSendResponse(uv_write_t* req, int32_t status)
{
    free(req);
}

void Channel::SendResponse(Controller* controller, const google::protobuf::Message* response)
{
    controller->meta_data().set_stub(false);

    std::map<int64_t, uv_stream_t*>::const_iterator it = connected_handle_.find(controller->fd());
    if (connected_handle_.end() == it) {
        WARN("connection:%ld, not connected", controller->fd());
        return;
    }

    std::string buffer;
    if (NULL != response)
    {
        std::string* message = controller->meta_data().mutable_message();
        if (NULL == message)
        {
            WARN("no more memory");
            return;
        }
        response->SerializeToString(message);
    }
    int32_t controller_nl = ::htonl(controller->meta_data().ByteSize());
    buffer.append((const char*)&controller_nl, sizeof(controller_nl));
    controller->meta_data().AppendToString(&buffer);

    uv_buf_t uv_buf;
    uv_buf.base = (char*)buffer.c_str();
    uv_buf.len = buffer.length();
    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    int error = uv_write(req, it->second, &uv_buf, 1, AfterSendResponse);
    if (error) {
        WARN("%s", uv_strerror(error));
        free(req);
    }
}

void Channel::OnRead(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
    Channel * self = (Channel*)handle->data;
    INFO("connection: %ld, nread: %ld", (int64_t)handle, nread);
    if (nread < 0) {
        self->CloseConnection(handle);
        return;
    }
    self->Handle(handle, nread);
}

void Channel::Handle(uv_stream_t* handle, ssize_t nread)
{
    Buffer& buffer = buffer_[(int64_t)handle];
    buffer.len += nread;
    ASSERT(buffer.len <= buffer.total, "out of memory");

    // handle
    size_t handled_len = 0;
    while (buffer.len >= sizeof(uint32_t) + handled_len){
        size_t buffer_cur = handled_len;

        int32_t controller_length_nl = 0;
        memcpy(&controller_length_nl, (int8_t*)buffer.base + buffer_cur, sizeof(controller_length_nl));
        buffer_cur += sizeof(controller_length_nl);
        size_t controller_length = ::ntohl(controller_length_nl);

        if (controller_length > controller_max_length_) {
            buffer_cur = buffer.len;
            WARN("connection: %ld, controller_length: %ld, out of max length: %ld", (int64_t)handle, controller_length, controller_max_length_);
            break;
        }

        if (buffer.len < controller_length + buffer_cur){
            break; // lack data
        }

        Controller* controller = new Controller();
        if (NULL == controller) {
            break;
        }

        if (!controller->meta_data().ParseFromArray((int8_t*)buffer.base + buffer_cur, controller_length)){
            buffer_cur += controller_length;
            break;
        }
        buffer_cur += controller_length;

        int32_t result = 0;

        controller->set_fd((int64_t)handle);
        if (controller->meta_data().stub()) {
            if (controller->meta_data().notify()) {
                result = HandleNotify(controller);
                delete controller;
            } else {
                result = HandleRequest(controller);
                //delete controller in closure
            }
        } else {
            result = HandleResponse(controller);
            delete controller;
        }

        if (-2 == result) {
            break;
        }
        handled_len = buffer_cur;
    }

    // move
    ASSERT(handled_len <= buffer.len, "libmaid: overflowed");
    buffer.len -= handled_len;
    ::memmove(buffer.base, (int8_t*)buffer.base + handled_len, buffer.len);
}

int32_t Channel::HandleRequest(Controller* controller)
{
    google::protobuf::Message* request = NULL;
    google::protobuf::Message* response = NULL;
    RemoteClosure* done = NewRemoteClosure();
    if (NULL == done) {
        return -2;
    }
    done->set_controller(controller);

    // service method
    std::map<std::string, google::protobuf::Service*>::iterator service_it;
    service_it = service_.find(controller->meta_data().full_service_name());
    if (service_.end() == service_it){
        WARN("service: %s, not exist", controller->meta_data().full_service_name().c_str());
        controller->SetFailed("libmaid: service not exist");
        done->Run();
        return -1;
    }
    google::protobuf::Service* service = service_it->second;
    const google::protobuf::ServiceDescriptor* service_descriptor = service->GetDescriptor();
    const google::protobuf::MethodDescriptor* method_descriptor = NULL;
    method_descriptor = service_descriptor->FindMethodByName(controller->meta_data().method_name());
    if (NULL == method_descriptor){
        WARN("service: %s, method: %s, not exist", controller->meta_data().full_service_name().c_str(), controller->meta_data().method_name().c_str());
        controller->SetFailed("libmaid: method not exist");
        done->Run();
        return -1;
    }

    // request
    request = service->GetRequestPrototype(method_descriptor).New();
    if (NULL == request){
        return -2; // delay
    }
    done->set_request(request);
    if (!request->ParseFromString(controller->meta_data().message())) {
        controller->SetFailed("parse failed");
        done->Run();
        return -1;
    }

    // response
    response = service->GetResponsePrototype(method_descriptor).New();
    done->set_response(response);

    // call
    service->CallMethod(method_descriptor, controller, request, response, done);
    return 0;
}

int32_t Channel::HandleResponse(Controller* controller)
{
    std::map<int64_t, Closure*>::iterator it = async_result_.find(controller->meta_data().transmit_id());
    if (it == async_result_.end() || it->second->controller()->fd() != controller->fd()) {
        return 0;
    }
    if (controller->Failed()){
        it->second->controller()->SetFailed(controller->meta_data().error_text());
    } else if (NULL != it->second->response()) {
        it->second->response()->ParseFromString(controller->meta_data().message());
    }
    async_result_.erase(it->first);
    transactions_[controller->fd()].erase(it->first);
    it->second->Run();
    return 0;
}

int32_t Channel::HandleNotify(Controller* controller)
{
    google::protobuf::Message* request = NULL;

    // service method
    std::map<std::string, google::protobuf::Service*>::iterator service_it;
    service_it = service_.find(controller->meta_data().full_service_name());
    if (service_.end() == service_it){
        return 0;
    }
    google::protobuf::Service* service = service_it->second;
    const google::protobuf::ServiceDescriptor* service_descriptor = service->GetDescriptor();
    const google::protobuf::MethodDescriptor* method_descriptor = NULL;
    method_descriptor = service_descriptor->FindMethodByName(controller->meta_data().method_name());
    if (NULL == method_descriptor){
        return 0;
    }

    // request
    request = service->GetRequestPrototype(method_descriptor).New();
    if (NULL == request){
        return -2; // delay
    }
    if (!request->ParseFromString(controller->meta_data().message())) {
        return 0;
    }

    // call
    service->CallMethod(method_descriptor, controller, request, NULL, NULL);
    return 0;
}

maid::closure::RemoteClosure* Channel::NewRemoteClosure()
{
    RemoteClosure* done = NULL;
    if (!closure_pool_.empty()) {
        done = closure_pool_.top();
        closure_pool_.pop();
    } else {
        done = new RemoteClosure(this);
    }
    return done;
}

void Channel::DestroyRemoteClosure(maid::closure::RemoteClosure* done)
{
    closure_pool_.push(done);
}

void Channel::OnConnect(uv_connect_t* req, int32_t status)
{
    Channel* self = (Channel*)req->handle->data;
    if (status) {
        self->CloseConnection(req->handle);
    } else {
        self->NewConnection((uv_stream_t*)req->handle);
    }
    free(req);
}

int64_t Channel::Connect(const char* host, int32_t port, bool as_default)
{
    uv_tcp_t* handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop_, handle);
    uv_tcp_nodelay(handle, 1);
    handle->data = this;

    struct sockaddr_in address;
    uv_ip4_addr(host, port, &address);
    uv_connect_t* req = (uv_connect_t*)malloc(sizeof(uv_connect_t));

    int result = uv_tcp_connect(req, handle, (struct sockaddr*)&address, OnConnect);
    if (result) {
        uv_close((uv_handle_t*)handle, OnClose);
        free(req);
        WARN("%s", uv_strerror(result));
        return result;
    }

    if (as_default) {
        default_fd_ = (int64_t)handle;
    }

    return (int64_t)handle;
}

void Channel::OnAccept(uv_stream_t* handle, int32_t status)
{
    Channel* self = (Channel*)handle->data;
    if (!status) {
        uv_tcp_t* peer_handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
        peer_handle->data = self;
        uv_tcp_init(handle->loop, peer_handle);
        int result = uv_accept(handle, (uv_stream_t*)peer_handle);
        if (result) {
            self->CloseConnection((uv_stream_t*)peer_handle);
            WARN("%s", uv_strerror(result));
            return;
        }
        self->NewConnection((uv_stream_t*)peer_handle);
    }
}

int64_t Channel::Listen(const char* host, int32_t port, int32_t backlog)
{
    uv_tcp_t* handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop_, handle);
    handle->data = this;
    struct sockaddr_in address;
    uv_ip4_addr(host, port, &address);

    int32_t flags = 0;
    int32_t result = -1;
    result = uv_tcp_bind(handle, (struct sockaddr*)&address, flags);
    if (result){
        uv_close((uv_handle_t*)handle, OnClose);
        WARN("%s", uv_strerror(result));
        return result;
    }
    result = uv_listen((uv_stream_t*)handle, backlog, OnAccept);
    if (result){
        uv_close((uv_handle_t*)handle, OnClose);
        WARN("%s", uv_strerror(result));
        return result;
    }

    handle->data = this;
    listen_handle_[(int64_t)handle] = (uv_stream_t*)handle;
    return (int64_t)handle;
}

void Channel::OnClose(uv_handle_t* handle)
{
    free(handle);
}

void Channel::CloseConnection(uv_stream_t* handle)
{
    INFO("close connection:%ld", (int64_t)handle);
    uv_read_stop(handle);
    std::set<int64_t>& transactions = transactions_[(int64_t)handle];
    for (std::set<int64_t>::const_iterator it = transactions.begin(); it != transactions.end(); ++it) {
        std::map<int64_t, maid::closure::Closure*>::iterator async_it = async_result_.find(*it);
        async_it->second->controller()->SetFailed("connection closed");
        async_it->second->Run();
        async_result_.erase(*it);
    }
    transactions_.erase((int64_t)handle);
    connected_handle_.erase((int64_t)handle);
    buffer_.erase((int64_t)handle);

    handle->data = this;
    uv_close((uv_handle_t*)handle, OnClose);
}

void Channel::NewConnection(uv_stream_t* handle)
{
    INFO("new connection:%ld", (int64_t)handle);
    handle->data = this;
    connected_handle_[(int64_t)handle] = handle;
    uv_read_start(handle, OnAlloc, OnRead);
}

void Channel::OnAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    Channel* self = (Channel*)handle->data;
    Buffer& buffer = self->buffer_[(int64_t)handle];
    buffer.Expend(suggested_size);

    buf->base = (char*)buffer.base + buffer.len ;
    buf->len = buffer.total - buffer.len;
}

void Channel::OnRemoteClosureGC(uv_idle_t* handle)
{
}

void Buffer::Expend(size_t expect_length)
{
    size_t new_length = total;
    while (new_length <= expect_length + len){
        new_length += 1;
        new_length <<= 1;
        void* new_ptr = ::realloc(base, new_length);
        if (NULL == new_ptr){
            ERROR("expend failed");
            return;
        }
        base = new_ptr;
    }
    //::memset(*ptr + (*origin_length * type_size), 0, (new_length - *origin_length) * type_size);
    total = new_length;
}
