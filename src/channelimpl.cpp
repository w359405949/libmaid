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
     default_connect_(0),
     transmit_id_max_(0)
{
    signal(SIGPIPE, SIG_IGN);
    loop_ = loop;
    if (NULL == loop_) {
        loop_ = uv_default_loop();
    }
}

ChannelImpl::~ChannelImpl()
{
    std::map< std::string, google::protobuf::Service*>::iterator it;
    for (it = service_.begin(); it != service_.end(); it++) {
        delete it->second;
    }
    if (uv_default_loop() == loop_) {
        uv_loop_delete(loop_);
    }
    loop_ = NULL;
}

void ChannelImpl::AppendService(google::protobuf::Service* service)
{
    GOOGLE_LOG_IF(FATAL, NULL == service) << " service can not be NULL ";
    GOOGLE_LOG_IF(FATAL, service_.find(service->GetDescriptor()->full_name()) != service_.end()) << " same name service regist twice";

    service_[service->GetDescriptor()->full_name()] = service;
}

void ChannelImpl::CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* rpc_controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done)
{
    GOOGLE_LOG_IF(FATAL, NULL == dynamic_cast<Controller*>(rpc_controller)) << " controller must have type maid::Controller";
    GOOGLE_LOG_IF(FATAL, NULL == request) << " should not be NULL";

    Controller* controller = (Controller*)rpc_controller;
    if (!controller->fd()) {
        controller->set_fd(default_connect_);
    }
    controller->meta_data().set_full_service_name(method->service()->full_name());
    controller->meta_data().set_method_name(method->name());

    if (controller->meta_data().notify()) {
        GOOGLE_LOG_IF(FATAL, NULL != done) << " notify request will never called the callback";
        GOOGLE_LOG_IF(FATAL, NULL != response) << " notify message will not recived a response";
        SendNotify(controller, request);
    } else {
        SendRequest(controller, request, response, done);
    }
}

void ChannelImpl::AfterSendNotify(uv_write_t* req, int32_t status)
{
    GOOGLE_LOG(INFO) << " connection: " << (int64_t)req->handle << " after send notify: " << status;
    free(req);
}

void ChannelImpl::SendNotify(Controller* controller, const google::protobuf::Message* request)
{
    controller->meta_data().set_stub(true);
    std::map<int64_t, uv_stream_t*>::const_iterator it = connected_handle_.find(controller->fd());
    if (connected_handle_.end() == it) {
        GOOGLE_LOG(INFO) << " not connected ";
        return;
    }

    std::string buffer;
    try {
        request->SerializeToString(controller->meta_data().mutable_message());
        int32_t controller_nl = ::htonl(controller->meta_data().ByteSize());
        buffer.append((const char*)&controller_nl, sizeof(controller_nl));
        controller->meta_data().AppendToString(&buffer);
    } catch (std::bad_alloc) {
        return;
    }

    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    req->data = controller;

    uv_buf_t uv_buf;
    uv_buf.base = (char*)buffer.data();
    uv_buf.len = buffer.length();
    int error = uv_write(req, it->second, &uv_buf, 1, AfterSendNotify);
    if (error) {
        GOOGLE_LOG(INFO) << uv_strerror(error);
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
    GOOGLE_LOG_IF(FATAL, NULL == done) << " done should not be NULL";
    GOOGLE_LOG_IF(FATAL, NULL == response) << " response should not be NULL";
    controller->meta_data().set_stub(true);
    std::map<int64_t, uv_stream_t*>::const_iterator it = connected_handle_.find(controller->fd());
    if (it == connected_handle_.end()) {
        controller->SetFailed("not connected");
        done->Run();
        return;
    }

    uint32_t transmit_id = ++transmit_id_max_; // TODO: check if used

    controller->meta_data().set_transmit_id(transmit_id);
    async_result_[transmit_id].controller = controller;
    async_result_[transmit_id].response = response;
    async_result_[transmit_id].done = done;
    transactions_[controller->fd()].insert(transmit_id);

    std::string buffer;
    try {
        request->SerializeToString(controller->meta_data().mutable_message());
        int32_t controller_nl = ::htonl(controller->meta_data().ByteSize());
        buffer.append((const char*)&controller_nl, sizeof(controller_nl));
        controller->meta_data().AppendToString(&buffer);
    } catch (std::bad_alloc) {
        controller->SetFailed("busy");
        HandleResponse(controller);
        return;
    }

    GOOGLE_LOG(INFO) << " connection: " << controller->fd() << " send request: " << controller->meta_data().transmit_id();

    uv_buf_t uv_buf;
    uv_buf.base = (char*)buffer.data();
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
    GOOGLE_LOG_IF(FATAL, status != 0) << "after send response";
    free(req);
}

void ChannelImpl::SendResponse(Controller* controller, const google::protobuf::Message* response)
{
    controller->meta_data().set_stub(false);

    std::map<int64_t, uv_stream_t*>::const_iterator it = connected_handle_.find(controller->fd());
    if (connected_handle_.end() == it) {
        GOOGLE_LOG(WARNING) << " connection: " << controller->fd() << " not connected";
        return;
    }

    std::string buffer;
    controller->meta_data().clear_message();
    if (NULL != response) {
        try{
            response->SerializeToString(controller->meta_data().mutable_message());
            int32_t controller_nl = ::htonl(controller->meta_data().ByteSize());
            buffer.append((char*)&controller_nl, sizeof(controller_nl));
            controller->meta_data().AppendToString(&buffer);
        } catch (std::bad_alloc) {
        }
    }

    GOOGLE_LOG(INFO) << " connection: " << controller->fd() << " send response: " << controller->meta_data().transmit_id();

    uv_buf_t uv_buf;
    uv_buf.base = (char*)buffer.data();
    uv_buf.len = buffer.length();
    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    GOOGLE_LOG_IF(FATAL, NULL == req) << " no more memory";
    int error = uv_write(req, it->second, &uv_buf, 1, AfterSendResponse);
    if (error) {
        GOOGLE_LOG(FATAL) << uv_strerror(error);
        free(req);
    }
}

void ChannelImpl::OnRead(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
    ChannelImpl * self = (ChannelImpl*)handle->data;
    GOOGLE_LOG(INFO) << " connection: " << (int64_t)handle << " nread: " << nread << " buf->len: " << buf->len;

    if (nread < 0) {
        self->RemoveConnection(handle);
        return;
    }

    Buffer& buffer = self->buffer_[(int64_t)(handle)];
    GOOGLE_LOG_IF(FATAL, buffer.len + nread > buffer.total) << " out of memory";
    if (buf->base != buffer.base) {
        memcpy(((int8_t*)buffer.base) + buffer.len, buf->base, nread);
    }
    buffer.len += nread;

    self->Handle(handle, buffer);
}

int32_t ChannelImpl::Handle(uv_stream_t* handle, Buffer& buffer)
{
    // handle
    int32_t result = 0;
    size_t handled_len = 0;
    while (buffer.len >= sizeof(int32_t) + handled_len) {
        size_t buffer_cur = handled_len;

        int32_t controller_length_nl = 0;
        memcpy(&controller_length_nl, ((int8_t*)buffer.base) + buffer_cur, sizeof(controller_length_nl));
        int32_t controller_length = ::ntohl(controller_length_nl);
        buffer_cur += sizeof(controller_length);
        Controller* controller = NULL;

        if (controller_length > controller_max_length_ || controller_length < 0) {
            handled_len = buffer.len;
            GOOGLE_LOG(FATAL) << " connection: " << (int64_t)handle << " controller_length: " << controller_length << " out of max length: " << controller_max_length_;
            result = ERROR_OUT_OF_LENGTH; // out of max length
            break;
        }

        if (buffer.len < controller_length + buffer_cur) {
            result = ERROR_LACK_DATA;
            break; // lack data
        }

        try {
            controller = new Controller();
            if (!controller->meta_data().ParseFromArray((int8_t*)buffer.base + buffer_cur, controller_length)) {
                GOOGLE_LOG(FATAL) << " connection: " << (int64_t)handle << " parse meta_data failed";
                handled_len += sizeof(uint32_t) + controller_length;
                delete controller;
                continue;
            }

            controller->set_fd((int64_t)(handle));

        } catch (std::bad_alloc) {
            delete controller;
            result = ERROR_BUSY;
            break;
        }

        if (controller->meta_data().method_name() == RESERVED_METHOD_CONNECT) {
            //WARN("method: %s is a reserved method, do not call it remotely", RESERVED_METHOD_CONNECT);
            handled_len = buffer_cur;
            continue;
        }

        if (controller->meta_data().method_name() == RESERVED_METHOD_DISCONNECT) {
            //WARN("method: %s is a reserved method, do not call it remotely", RESERVED_METHOD_DISCONNECT);
            handled_len = buffer_cur;
            continue;
        }

        if (controller->meta_data().stub()) {
            if (controller->meta_data().notify()) {
                result = HandleNotify(controller);
            } else {
                result = HandleRequest(controller);
            }
        } else {
            result = HandleResponse(controller);
        }

        if (ERROR_BUSY == result) { // busy
            break;
        }

        handled_len = buffer_cur + controller_length;
    }

    GOOGLE_LOG_IF(FATAL, handled_len > buffer.len) << " overflowed";
    // move
    buffer.len -= handled_len;
    if (buffer.len != 0)
    {
        GOOGLE_LOG(INFO) << " buffer.len: " << buffer.len << " handled_len: " << handled_len << " total: " << buffer.total;
        memmove(buffer.base, ((int8_t*)buffer.base) + handled_len, buffer.len);
    }
    return result;
}

int32_t ChannelImpl::HandleRequest(Controller* controller)
{
    GOOGLE_LOG(INFO) << " connection: " << controller->fd() << " request: " << controller->meta_data().transmit_id();

    // service method
    std::map<std::string, google::protobuf::Service*>::iterator service_it;
    service_it = service_.find(controller->meta_data().full_service_name());
    if (service_.end() == service_it) {
        GOOGLE_LOG(WARNING) << " service: " << controller->meta_data().full_service_name().c_str() << " not exist";
        delete controller;
        return ERROR_OTHER;
    }
    google::protobuf::Service* service = service_it->second;
    const google::protobuf::MethodDescriptor* method_descriptor = NULL;
    method_descriptor = service->GetDescriptor()->FindMethodByName(controller->meta_data().method_name());
    if (NULL == method_descriptor) {
        GOOGLE_LOG(WARNING) << " service: " << controller->meta_data().full_service_name().c_str() << " method: " << controller->meta_data().method_name() << " not exist";
        delete controller;
        return ERROR_OTHER;
    }

    google::protobuf::Message* request = NULL;
    google::protobuf::Message* response = NULL;
    RemoteClosure* done = NULL;
    try {
        request = service->GetRequestPrototype(method_descriptor).New();
        response = service->GetResponsePrototype(method_descriptor).New();
        done = NewRemoteClosure();

        if (!request->ParseFromString(controller->meta_data().message())) {
            delete request;
            delete response;
            delete done;
            delete controller;
            return ERROR_PARSE_FAILED;
        }

        done->set_controller(controller);
        done->set_request(request);
        done->set_response(response);

    } catch (std::bad_alloc) {
        delete request;
        delete response;
        delete done;
        delete controller;
        return ERROR_BUSY;
    }

    service->CallMethod(method_descriptor, controller, request, response, done);

    // call
    return 0;
}

int32_t ChannelImpl::HandleResponse(Controller* controller)
{
    GOOGLE_LOG(INFO) << " connection: " << controller->fd() << " response: " << controller->meta_data().transmit_id() << " size: " << controller->meta_data().ByteSize();

    std::map<int64_t, Context>::iterator it = async_result_.find(controller->meta_data().transmit_id());
    if (it == async_result_.end() || it->second.controller->fd() != controller->fd()) {
        delete controller;
        return 0;
    }
    if (controller->Failed()) {
        it->second.controller->SetFailed(controller->ErrorText());
    } else {
        it->second.response->ParseFromString(controller->meta_data().message());
    }
    it->second.done->Run();
    async_result_.erase(it->first);
    transactions_[controller->fd()].erase(it->first);
    if (transactions_[controller->fd()].size() == 0) {
        transactions_.erase(controller->fd());
    }
    return 0;
}

int32_t ChannelImpl::HandleNotify(Controller* controller)
{
    GOOGLE_LOG(INFO) << " connection: " << controller->fd() << " notify: " << controller->meta_data().transmit_id() << " size: " << controller->meta_data().ByteSize();

    // service method
    std::map<std::string, google::protobuf::Service*>::iterator service_it;
    service_it = service_.find(controller->meta_data().full_service_name());
    if (service_.end() == service_it) {
        delete controller;
        return 0;
    }
    google::protobuf::Service* service = service_it->second;
    const google::protobuf::ServiceDescriptor* service_descriptor = service->GetDescriptor();
    const google::protobuf::MethodDescriptor* method_descriptor = NULL;
    method_descriptor = service_descriptor->FindMethodByName(controller->meta_data().method_name());
    if (NULL == method_descriptor) {
        delete controller;
        return 0;
    }

    google::protobuf::Message* request = NULL;
    try {
        request = service->GetRequestPrototype(method_descriptor).New();
        if (!request->ParseFromString(controller->meta_data().message())) {
            delete controller;
            return 0;
        }
    } catch (std::bad_alloc) {
        delete request;
        delete controller;
        return ERROR_BUSY; // delay
    }

    service->CallMethod(method_descriptor, controller, request, NULL, NULL);
    delete controller;
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
    if (done != NULL) {
        remote_closure_pool_.push(done);
    }
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
        GOOGLE_LOG(WARNING) << uv_strerror(result);
        return ERROR_OTHER;
    }

    if (as_default) {
        default_connect_ = (int64_t)(handle);
    }

    return (int64_t)(handle);
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
        GOOGLE_LOG(WARNING) << uv_strerror(result);
        uv_close((uv_handle_t*)peer_handle, OnClose);
        return;
    }

    uv_tcp_nodelay(peer_handle, 1);
    self->AddConnection((uv_stream_t*)peer_handle);
}

int64_t ChannelImpl::Listen(const char* host, int32_t port, int32_t backlog)
{
    uv_tcp_t* handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop_, handle);
    struct sockaddr_in address;
    int32_t result = 0;
    result = uv_ip4_addr(host, port, &address);
    if (result) {
        uv_close((uv_handle_t*)handle, OnClose);
        GOOGLE_LOG(WARNING) << uv_strerror(result);
        return ERROR_OTHER;
    }

    int32_t flags = 0;
    result = uv_tcp_bind(handle, (struct sockaddr*)&address, flags);
    if (result) {
        uv_close((uv_handle_t*)handle, OnClose);
        GOOGLE_LOG(WARNING) << uv_strerror(result);
        return ERROR_OTHER;
    }
    result = uv_listen((uv_stream_t*)handle, backlog, OnAccept);
    if (result) {
        uv_close((uv_handle_t*)handle, OnClose);
        GOOGLE_LOG(WARNING) << uv_strerror(result);
        return ERROR_OTHER;
    }

    handle->data = this;
    listen_handle_[(int64_t)handle] = (uv_stream_t*)handle;
    return (int64_t)handle;
}

void ChannelImpl::OnClose(uv_handle_t* handle)
{
    handle->data = NULL;
    free(handle);
}

void ChannelImpl::RemoveConnection(uv_stream_t* handle)
{
    GOOGLE_LOG(INFO) << " close connection: " << (int64_t)handle;

    uv_read_stop(handle);
    //std::set<int64_t>& transactions = transactions_[(int64_t)handle];
    //for (std::set<int64_t>::const_iterator it = transactions.begin(); it != transactions.end(); ++it) {
    //    std::map<int64_t, Context>::iterator async_it = async_result_.find(*it);
    //    async_it->second.controller->SetFailed("connection closed");
    //    async_it->second.done->Run();
    //    async_result_.erase(*it);
    //}
    //transactions_.erase((int64_t)(handle));
    connected_handle_.erase((int64_t)(handle));
    buffer_.erase((int64_t)(handle));

    handle->data = this;
    uv_close((uv_handle_t*)handle, OnClose);

    /*
    std::map<std::string, google::protobuf::Service*>::iterator it = service_.begin();
    for(;it != service_.end(); it++) {
        const google::protobuf::ServiceDescriptor* service_descriptor = it->second->GetDescriptor();
        const google::protobuf::MethodDescriptor* method_descriptor = NULL;
        method_descriptor = service_descriptor->FindMethodByName(RESERVED_METHOD_DISCONNECT);
        if (NULL == method_descriptor) {
            continue;
        }
        Controller controller;
        controller.set_fd((int64_t)(handle));
        it->second->CallMethod(method_descriptor, &controller, NULL, NULL, NULL);
    }
    */
}

void ChannelImpl::AddConnection(uv_stream_t* handle)
{
    GOOGLE_LOG(INFO) << " new connection: " << (int64_t)handle;

    handle->data = this;
    connected_handle_[(int64_t)(handle)] = handle;
    uv_read_start(handle, OnAlloc, OnRead);

    /*
    std::map<std::string, google::protobuf::Service*>::iterator it = service_.begin();
    for(;it != service_.end(); it++) {
        const google::protobuf::ServiceDescriptor* service_descriptor = it->second->GetDescriptor();
        const google::protobuf::MethodDescriptor* method_descriptor = NULL;
        method_descriptor = service_descriptor->FindMethodByName(RESERVED_METHOD_CONNECT);
        if (NULL == method_descriptor) {
            continue;
        }
        Controller controller;
        controller.set_fd((int64_t)(handle));
        it->second->CallMethod(method_descriptor, &controller, NULL, NULL, NULL);
    }
    */
}

void ChannelImpl::OnAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    ChannelImpl* self = (ChannelImpl*)handle->data;
    Buffer& buffer = self->buffer_[(int64_t)(handle)];
    buffer.Expend(suggested_size);

    buf->base = (char*)buffer.base + buffer.len ;
    buf->len = buffer.total - buffer.len;
    //buf->base = (char*)malloc(suggested_size);
    //buf->len = suggested_size;
}

void ChannelImpl::OnRemoteClosureGC(uv_idle_t* handle)
{
}
