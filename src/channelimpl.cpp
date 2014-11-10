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

#include "maid/controller.h"
#include "controller.pb.h"
#include "channelimpl.h"
#include "remoteclosure.h"
#include "define.h"
#include "buffer.h"
#include "context.h"

using maid::proto::ControllerMeta;
using maid::ChannelImpl;

ChannelImpl::ChannelImpl(uv_loop_t* loop)
    :controller_max_length_(CONTROLLERMETA_MAX_LENGTH),
     default_fd_(0)
{
    signal(SIGPIPE, SIG_IGN);
    if (NULL == loop)
    {
        loop = uv_loop_new();
    }
    loop_ = loop;
    //uv_idle_init(loop, &remote_closure_gc_);
    //uv_idle_start(&remote_closure_gc_, OnRemoteClosureGC);
}

ChannelImpl::~ChannelImpl()
{
    //uv_idle_stop(&remote_closure_gc_);
}

void ChannelImpl::AppendService(google::protobuf::Service* service)
{
    ASSERT(NULL != service, "service can not be NULL");
    ASSERT(service_.find(service->GetDescriptor()->full_name()) == service_.end(), "same name service regist twice");

    service_[service->GetDescriptor()->full_name()] = service;
}

void ChannelImpl::CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* rpc_controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done)
{
    ASSERT(dynamic_cast<Controller*>(rpc_controller), "controller must have type maid::Controller");
    ASSERT(request, "should not be NULL");

    Controller* controller = (Controller*)rpc_controller;
    if (!controller->fd()) {
        controller->set_fd(default_fd_);
    }
    controller->meta_data().set_full_service_name(method->service()->full_name());
    controller->meta_data().set_method_name(method->name());

    if (controller->meta_data().notify()) {
        ASSERT(NULL == done, "notify request will never called the callback");
        ASSERT(NULL == response, "notify message will not recived a response");
        SendNotify(controller, request);
    } else {
        SendRequest(controller, request, response, done);
    }
}

void ChannelImpl::AfterSendNotify(uv_write_t* req, int32_t status)
{
    INFO("connection: %ld, after send notify: %d", StreamToFd(req->handle), status);
    free(req);
}

void ChannelImpl::SendNotify(Controller* controller, const google::protobuf::Message* request)
{
    controller->meta_data().set_stub(true);
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

void ChannelImpl::AfterSendRequest(uv_write_t* req, int32_t status)
{
    if (status) {
        Controller* controller = (Controller*)req->data;
        ChannelImpl* self = (ChannelImpl*)req->handle->data;
        controller->SetFailed(uv_strerror(status));
        self->HandleResponse(controller);
    }
    free(req);
}

void ChannelImpl::SendRequest(Controller* controller, const google::protobuf::Message* request, google::protobuf::Message* response, google::protobuf::Closure* done)
{
    ASSERT(done, "callback should not be NULL");
    ASSERT(response, "response should not be NULL");
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
    async_result_[transmit_id].controller = controller;
    async_result_[transmit_id].response = response;
    async_result_[transmit_id].done = done;
    transactions_[controller->fd()].insert(transmit_id);

    std::string buffer;
    std::string* message = controller->meta_data().mutable_message(); //TODO
    request->SerializeToString(message);
    int32_t controller_nl = ::htonl(controller->meta_data().ByteSize());
    buffer.append((const char*)&controller_nl, sizeof(controller_nl));
    controller->meta_data().AppendToString(&buffer);

    INFO("connection: %ld, send request:%u", controller->fd(), controller->meta_data().transmit_id());

    uv_buf_t uv_buf;
    uv_buf.base = (char*)buffer.c_str();
    uv_buf.len = buffer.length();
    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    req->data = controller;
    int error = uv_write(req, it->second, &uv_buf, 1, AfterSendRequest);
    if (error) {
        controller->SetFailed("send failed");
        HandleResponse(controller);
        free(req);
    }
}

void ChannelImpl::AfterSendResponse(uv_write_t* req, int32_t status)
{
    INFO("connection: %ld, after send response: %d", StreamToFd(req->handle), status);
    free(req);
}

void ChannelImpl::SendResponse(Controller* controller, const google::protobuf::Message* response)
{
    controller->meta_data().set_stub(false);

    std::map<int64_t, uv_stream_t*>::const_iterator it = connected_handle_.find(controller->fd());
    if (connected_handle_.end() == it) {
        WARN("connection: %ld, not connected", controller->fd());
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

    INFO("connection: %ld, send response:%u", controller->fd(), controller->meta_data().transmit_id());

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

void ChannelImpl::OnRead(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
    ChannelImpl * self = (ChannelImpl*)handle->data;
    INFO("connection: %ld, nread: %ld", StreamToFd(handle), nread);
    if (nread < 0) {
        self->RemoveConnection(handle);
        return;
    }
    self->Handle(handle, nread);
}

int32_t ChannelImpl::Handle(uv_stream_t* handle, ssize_t nread)
{
    Buffer& buffer = buffer_[StreamToFd(handle)];
    buffer.len += nread;
    ASSERT(buffer.len <= buffer.total, "out of memory");

    int32_t result = 0;
    // handle
    size_t handled_len = 0;
    while (buffer.len >= sizeof(uint32_t) + handled_len){
        size_t buffer_cur = handled_len;

        uint32_t controller_length_nl = 0;
        memcpy(&controller_length_nl, (uint8_t*)buffer.base + buffer_cur, sizeof(uint32_t));
        uint32_t controller_length = ::ntohl(controller_length_nl);
        buffer_cur += sizeof(uint32_t);

        if (controller_length > controller_max_length_) {
            handled_len = buffer.len;
            WARN("connection: %lu, controller_length: %u, out of max length: %u", StreamToFd(handle), controller_length, controller_max_length_);
            result = ERROR_OUT_OF_LENGTH; // out of max length
            break;
        }

        if (buffer.len < controller_length + buffer_cur){
            result = ERROR_LACK_DATA;
            break; // lack data
        }

        Controller* controller = new Controller();
        if (NULL == controller) {
            result = ERROR_BUSY; // busy
            break;
        }

        if (!controller->meta_data().ParseFromArray((int8_t*)buffer.base + buffer_cur, controller_length)){
            WARN("connection: %ld, parse meta_data failed", StreamToFd(handle));
            handled_len += sizeof(uint32_t) + controller_length;
            result = ERROR_PARSE_FAILED; // parse failed
            break;
        }
        buffer_cur += controller_length;

        controller->set_fd(StreamToFd(handle));
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

        if (ERROR_BUSY == result) { // busy
            break;
        }
        handled_len = buffer_cur;
    }

    // move
    ASSERT(handled_len <= buffer.len, "overflowed");
    buffer.len -= handled_len;
    if (buffer.len != 0)
    {
        memmove(buffer.base, (int8_t*)buffer.base + handled_len, buffer.len);
    }
    return result;
}

int32_t ChannelImpl::HandleRequest(Controller* controller)
{
    INFO("connection: %ld, request: %u, size: %d", controller->fd(), controller->meta_data().transmit_id(), controller->meta_data().ByteSize());
    google::protobuf::Message* request = NULL;
    google::protobuf::Message* response = NULL;
    RemoteClosure* done = NewRemoteClosure();
    if (NULL == done) {
        return ERROR_BUSY;
    }
    done->set_controller(controller);

    // service method
    std::map<std::string, google::protobuf::Service*>::iterator service_it;
    service_it = service_.find(controller->meta_data().full_service_name());
    if (service_.end() == service_it){
        WARN("service: %s, not exist", controller->meta_data().full_service_name().c_str());
        controller->SetFailed("service not exist");
        done->Run();
        return ERROR_OTHER;
    }
    google::protobuf::Service* service = service_it->second;
    const google::protobuf::ServiceDescriptor* service_descriptor = service->GetDescriptor();
    const google::protobuf::MethodDescriptor* method_descriptor = NULL;
    method_descriptor = service_descriptor->FindMethodByName(controller->meta_data().method_name());
    if (NULL == method_descriptor){
        WARN("service: %s, method: %s, not exist", controller->meta_data().full_service_name().c_str(), controller->meta_data().method_name().c_str());
        controller->SetFailed("method not exist");
        done->Run();
        return ERROR_OTHER;
    }

    // request
    request = service->GetRequestPrototype(method_descriptor).New();
    if (NULL == request){
        return ERROR_BUSY; // delay
    }
    done->set_request(request);
    if (!request->ParseFromString(controller->meta_data().message())) {
        controller->SetFailed("parse failed");
        done->Run();
        return ERROR_PARSE_FAILED;
    }

    // response
    response = service->GetResponsePrototype(method_descriptor).New();
    done->set_response(response);

    // call
    service->CallMethod(method_descriptor, controller, request, response, done);
    return 0;
}

int32_t ChannelImpl::HandleResponse(Controller* controller)
{
    INFO("connection: %ld, response: %u, size: %d", controller->fd(), controller->meta_data().transmit_id(), controller->meta_data().ByteSize());
    std::map<int64_t, Context>::iterator it = async_result_.find(controller->meta_data().transmit_id());
    if (it == async_result_.end() || it->second.controller->fd() != controller->fd()) {
        return 0;
    }
    if (controller->Failed()){
        it->second.controller->SetFailed(controller->ErrorText());
    } else {
        it->second.response->ParseFromString(controller->meta_data().message());
    }
    async_result_.erase(it->first);
    transactions_[controller->fd()].erase(it->first);
    it->second.done->Run();
    return 0;
}

int32_t ChannelImpl::HandleNotify(Controller* controller)
{
    INFO("connection: %ld, notify: %u, size: %d", controller->fd(), controller->meta_data().transmit_id(), controller->meta_data().ByteSize());
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
        return ERROR_BUSY; // delay
    }
    if (!request->ParseFromString(controller->meta_data().message())) {
        return 0;
    }

    // call
    service->CallMethod(method_descriptor, controller, request, NULL, NULL);
    return 0;
}

maid::RemoteClosure* ChannelImpl::NewRemoteClosure()
{
    RemoteClosure* done = NULL;
    if (!remote_closure_pool_.empty()) {
        done = remote_closure_pool_.top();
        remote_closure_pool_.pop();
    } else {
        done = new RemoteClosure(this);
    }
    return done;
}

void ChannelImpl::DeleteRemoteClosure(maid::RemoteClosure* done)
{
    remote_closure_pool_.push(done);
}

void ChannelImpl::OnConnect(uv_connect_t* req, int32_t status)
{
    ChannelImpl* self = (ChannelImpl*)req->data;
    if (!status) {
        self->AddConnection(req->handle);
    }
    free(req);
}

int64_t ChannelImpl::Connect(const char* host, int32_t port, bool as_default)
{
    uv_tcp_t* handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop_, handle);
    uv_tcp_nodelay(handle, 1);

    struct sockaddr_in address;
    uv_ip4_addr(host, port, &address);
    uv_connect_t* req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    req->data = this;

    int result = uv_tcp_connect(req, handle, (struct sockaddr*)&address, OnConnect);
    if (result) {
        uv_close((uv_handle_t*)handle, OnClose);
        free(req);
        WARN("%s", uv_strerror(result));
        return ERROR_OTHER;
    }

    if (as_default) {
        default_fd_ = StreamToFd((uv_stream_t*)handle);
    }

    return StreamToFd((uv_stream_t*)handle);
}

void ChannelImpl::OnAccept(uv_stream_t* handle, int32_t status)
{
    if (status) {
        return;
    }

    ChannelImpl* self = (ChannelImpl*)handle->data;
    uv_tcp_t* peer_handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(handle->loop, peer_handle);
    int result = uv_accept(handle, (uv_stream_t*)peer_handle);
    if (result) {
        WARN("%s", uv_strerror(result));
        uv_close((uv_handle_t*)peer_handle, OnClose);
        return;
    }
    self->AddConnection((uv_stream_t*)peer_handle);
}

int64_t ChannelImpl::Listen(const char* host, int32_t port, int32_t backlog)
{
    uv_tcp_t* handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop_, handle);
    struct sockaddr_in address;
    uv_ip4_addr(host, port, &address);

    int32_t flags = 0;
    int32_t result = 0;
    result = uv_tcp_bind(handle, (struct sockaddr*)&address, flags);
    if (result){
        uv_close((uv_handle_t*)handle, OnClose);
        WARN("%s", uv_strerror(result));
        return ERROR_OTHER;
    }
    result = uv_listen((uv_stream_t*)handle, backlog, OnAccept);
    if (result){
        uv_close((uv_handle_t*)handle, OnClose);
        WARN("%s", uv_strerror(result));
        return ERROR_OTHER;
    }

    handle->data = this;
    listen_handle_[StreamToFd((uv_stream_t*)handle)] = (uv_stream_t*)handle;
    return StreamToFd((uv_stream_t*)handle);
}

void ChannelImpl::OnClose(uv_handle_t* handle)
{
    handle->data = NULL;
    free(handle);
}

void ChannelImpl::RemoveConnection(uv_stream_t* handle)
{
    INFO("close connection: %ld", StreamToFd(handle));
    uv_read_stop(handle);
    std::set<int64_t>& transactions = transactions_[StreamToFd(handle)];
    for (std::set<int64_t>::const_iterator it = transactions.begin(); it != transactions.end(); ++it) {
        std::map<int64_t, Context>::iterator async_it = async_result_.find(*it);
        async_it->second.controller->SetFailed("connection closed");
        async_it->second.done->Run();
        async_result_.erase(*it);
    }
    transactions_.erase(StreamToFd(handle));
    connected_handle_.erase(StreamToFd(handle));
    buffer_.erase(StreamToFd(handle));

    handle->data = this;
    uv_close((uv_handle_t*)handle, OnClose);

    std::map<std::string, google::protobuf::Service*>::iterator it = service_.begin();
    for(;it != service_.end(); it++) {
        const google::protobuf::ServiceDescriptor* service_descriptor = it->second->GetDescriptor();
        const google::protobuf::MethodDescriptor* method_descriptor = NULL;
        method_descriptor = service_descriptor->FindMethodByName("Disconnect");
        if (NULL == method_descriptor){
            continue;
        }
        Controller controller;
        controller.set_fd(StreamToFd(handle));
        it->second->CallMethod(method_descriptor, &controller, NULL, NULL, NULL);
    }
}

void ChannelImpl::AddConnection(uv_stream_t* handle)
{
    INFO("new connection: %ld", StreamToFd(handle));
    handle->data = this;
    connected_handle_[StreamToFd(handle)] = handle;
    uv_read_start(handle, OnAlloc, OnRead);
}

void ChannelImpl::OnAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    ChannelImpl* self = (ChannelImpl*)handle->data;
    Buffer& buffer = self->buffer_[StreamToFd((uv_stream_t*)handle)];
    buffer.Expend(suggested_size);

    buf->base = (char*)buffer.base + buffer.len ;
    buf->len = buffer.total - buffer.len;
}

void ChannelImpl::OnRemoteClosureGC(uv_idle_t* handle)
{
}
