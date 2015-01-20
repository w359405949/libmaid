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

#include <glog/logging.h>

#include "maid/controller.h"
#include "controller.pb.h"
#include "options.pb.h"
#include "connection.pb.h"
#include "channelimpl.h"
#include "remoteclosure.h"
#include "define.h"
#include "buffer.h"
#include "context.h"
#include "mock.h"

using maid::ChannelImpl;

ChannelImpl::ChannelImpl(uv_loop_t* loop)
    :default_handle_(NULL),
     controller_max_length_(CONTROLLERMETA_MAX_LENGTH),
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

    uv_loop_close(loop_);
    loop_ = NULL;
}

void ChannelImpl::AppendService(google::protobuf::Service* service)
{
    DLOG_IF(FATAL, service_.find(service->GetDescriptor()->full_name()) != service_.end()) << " same name service regist twice";
    service_[service->GetDescriptor()->full_name()] = service;
}

void ChannelImpl::AppendConnectionEventService(proto::ConnectionEventService* event_service)
{
    connection_event_service_.push_back(event_service);
}

void ChannelImpl::CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* rpc_controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done)
{
    DLOG_IF(FATAL, NULL == dynamic_cast<Controller*>(rpc_controller)) << " controller must have type maid::Controller";
    DLOG_IF(FATAL, NULL == request) << " should not be NULL";

    Controller* controller = (Controller*)rpc_controller;
    if (controller->connection_id() == 0) {
        controller->set_connection_id((int64_t)default_handle_);
    }
    controller->mutable_proto()->set_full_service_name(method->service()->full_name());
    controller->mutable_proto()->set_method_name(method->name());

    const proto::MaidMethodOptions& method_options = method->options().GetExtension(proto::method_options);

    if (method_options.notify()) {
        DLOG_IF(FATAL, NULL != response) << method->full_name() << " notify=true, will never receive Response";
        DLOG_IF(FATAL, NULL != done) << method->full_name() << " notify=true, will never call Closure";
        SendNotify(controller, request);
    } else {
        SendRequest(controller, request, response, done);
    }
}

void ChannelImpl::AfterSendNotify(uv_write_t* req, int32_t status)
{
    DLOG(INFO) << " connection: " << (int64_t)req->handle << " after send notify: " << status;

    ChannelImpl* self = (ChannelImpl*)req->handle->data;

    delete self->sending_buffer_[req];
    self->sending_buffer_.erase(req);
    delete req;
}

void ChannelImpl::SendNotify(Controller* controller, const google::protobuf::Message* request)
{
    controller->mutable_proto()->set_stub(true);

    uv_stream_t* handle = connected_handle(controller);
    if (NULL == handle) {
        DLOG(INFO) << " not connected ";
        return;
    }

    std::string* buffer = NULL;
    uv_write_t* req = NULL;
    try {
        buffer = new std::string();
        req = new uv_write_t();

        request->SerializeToString(controller->mutable_proto()->mutable_message());
        int32_t controller_nl = htonl(controller->proto().ByteSize());
        buffer->append((const char*)&controller_nl, sizeof(controller_nl));
        controller->proto().AppendToString(buffer);

        sending_buffer_[req] = buffer;
    } catch (std::bad_alloc) {
        delete buffer;
        delete req;
        return;
    }


    req->data = controller;
    uv_buf_t uv_buf;
    uv_buf.base = (char*)buffer->data();
    uv_buf.len = buffer->length();
    int error = uv_write(req, handle, &uv_buf, 1, AfterSendNotify);
    if (error) {
        DLOG(INFO) << uv_strerror(error);
        free(req);
    }
}

void ChannelImpl::AfterSendRequest(uv_write_t* req, int32_t status)
{
    ChannelImpl* self = (ChannelImpl*)req->handle->data;

    if (status) {
        Controller* controller = (Controller*)req->data;
        controller->SetFailed(uv_strerror(status));
        self->HandleResponse(controller);
    }
    delete self->sending_buffer_[req];
    self->sending_buffer_.erase(req);
    delete req;
}

void ChannelImpl::SendRequest(Controller* controller, const google::protobuf::Message* request, google::protobuf::Message* response, google::protobuf::Closure* done)
{
    DLOG_IF(FATAL, NULL == done) << " done should not be NULL";
    DLOG_IF(FATAL, NULL == response) << " response should not be NULL";
    controller->mutable_proto()->set_stub(true);
    uv_stream_t* handle = connected_handle(controller);
    if (NULL == handle) {
        controller->SetFailed("not connected");
        done->Run();
        return;
    }

    uint32_t transmit_id = ++transmit_id_max_; // TODO: check if used
    DLOG_IF(FATAL, async_result_.find(transmit_id) != async_result_.end()) << "transmit id used";

    controller->mutable_proto()->set_transmit_id(transmit_id);
    async_result_[transmit_id].controller = controller;
    async_result_[transmit_id].response = response;
    async_result_[transmit_id].done = done;
    transactions_[controller->connection_id()].insert(transmit_id);

    std::string* buffer = NULL;
    uv_write_t* req = NULL;
    try {
        buffer = new std::string();
        req = new uv_write_t();

        request->SerializeToString(controller->mutable_proto()->mutable_message());
        int32_t controller_nl = htonl(controller->proto().ByteSize());
        buffer->append((const char*)&controller_nl, sizeof(controller_nl));
        controller->proto().AppendToString(buffer);

        sending_buffer_[req] = buffer;
    } catch (std::bad_alloc) {
        delete buffer;
        delete req;
        controller->SetFailed("busy");
        HandleResponse(controller);
        return;
    }

    DLOG(INFO) << " connection: " << controller->connection_id() << " send request: " << controller->proto().transmit_id();

    uv_buf_t uv_buf;
    uv_buf.base = (char*)buffer->data();
    uv_buf.len = buffer->length();
    req->data = controller;
    int error = uv_write(req, handle, &uv_buf, 1, AfterSendRequest);
    if (error) {
        controller->SetFailed("send failed");
        HandleResponse(controller);
        free(req);
    }
}

void ChannelImpl::AfterSendResponse(uv_write_t* req, int32_t status)
{
    ChannelImpl* self = (ChannelImpl*)req->handle->data;

    delete self->sending_buffer_[req];
    self->sending_buffer_.erase(req);
    delete req;
}

void ChannelImpl::SendResponse(Controller* controller, const google::protobuf::Message* response)
{
    controller->mutable_proto()->set_stub(false);

    uv_stream_t* handle = connected_handle(controller);
    if (NULL == handle) {
        DLOG(WARNING) << " connection: " << controller->connection_id() << " not connected";
        return;
    }

    controller->mutable_proto()->clear_message();
    std::string* buffer = NULL;
    uv_write_t* req = NULL;
    if (NULL != response) {
        try{
            buffer = new std::string();
            req = new uv_write_t();
            response->SerializeToString(controller->mutable_proto()->mutable_message());
            int32_t controller_nl = htonl(controller->proto().ByteSize());
            buffer->append((char*)&controller_nl, sizeof(controller_nl));
            controller->proto().AppendToString(buffer);

            sending_buffer_[req] = buffer;
        } catch (std::bad_alloc) {
            delete buffer;
            delete req;
            DLOG(FATAL) << " no more memory";
            return;
        }
    }

    DLOG(INFO) << " connection: " << controller->connection_id() << " send response: " << controller->proto().transmit_id();

    uv_buf_t uv_buf;
    uv_buf.base = (char*)buffer->data();
    uv_buf.len = buffer->length();
    int error = uv_write(req, handle, &uv_buf, 1, AfterSendResponse);
    if (error) {
        DLOG(FATAL) << uv_strerror(error);
        free(req);
    }
}

void ChannelImpl::OnRead(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
    ChannelImpl * self = (ChannelImpl*)handle->data;
    DLOG(INFO) << " connection: " << (int64_t)handle << " nread: " << nread << " buf->len: " << buf->len;

    if (nread < 0) {
        self->RemoveConnection(handle);
        return;
    }

    /*
     * TODO: buf->base alloced by libuv (win).
     */

    Buffer& buffer = self->buffer_[(int64_t)(handle)];
    DLOG_IF(FATAL, buffer.len + nread > buffer.total) << " out of memory";
    buffer.len += nread;
    self->Handle(handle, buffer);
}

int32_t ChannelImpl::Handle(uv_stream_t* handle, Buffer& buffer)
{
    // handle
    int32_t result = 0;
    int8_t* handled = (int8_t*)buffer.base;
    int8_t* buffer_end = handled + buffer.len;
    while (buffer_end >= handled + sizeof(int32_t)) {
        int8_t* buffer_cur = handled;

        int32_t controller_length_nl = 0;
        memcpy(&controller_length_nl, buffer_cur, sizeof(controller_length_nl));
        int32_t controller_length = ntohl(controller_length_nl);
        buffer_cur += sizeof(controller_length);
        Controller* controller = NULL;

        if (controller_length > controller_max_length_ || controller_length < 0) {
            handled = buffer_end;
            DLOG(FATAL) << " connection: " << (int64_t)handle << " controller_length: " << controller_length << " out of max length: " << controller_max_length_;
            result = ERROR_OUT_OF_LENGTH; // out of max length
            break;
        }

        if (buffer_cur + controller_length > buffer_end) {
            result = ERROR_LACK_DATA;
            break;
        }

        try {
            controller = new Controller();
            if (!controller->mutable_proto()->ParseFromArray(buffer_cur, controller_length)) {
                DLOG(FATAL) << " connection: " << (int64_t)handle << " parse ControllerProto failed";
                handled = buffer_cur + controller_length;
                delete controller;
                continue;
            }

            controller->set_connection_id((int64_t)(handle));

        } catch (std::bad_alloc) {
            delete controller;
            result = ERROR_BUSY;
            break;
        }

        if (controller->proto().stub()) {
            // service method
            std::map<std::string, google::protobuf::Service*>::iterator service_it;
            service_it = service_.find(controller->proto().full_service_name());
            if (service_.end() == service_it) {
                DLOG(WARNING) << " service: " << controller->proto().full_service_name().c_str() << " not exist";
                delete controller;
                result = ERROR_OTHER;
                break;
            }

            google::protobuf::Service* service = service_it->second;
            const google::protobuf::MethodDescriptor* method_descriptor = NULL;
            method_descriptor = service->GetDescriptor()->FindMethodByName(controller->proto().method_name());
            if (NULL == method_descriptor) {
                DLOG(WARNING) << " service: " << controller->proto().full_service_name().c_str() << " method: " << controller->proto().method_name() << " not exist";
                delete controller;
                result = ERROR_OTHER;
                break;
            }

            const proto::MaidMethodOptions& method_options = method_descriptor->options().GetExtension(proto::method_options);

            if (method_options.notify()) {
                result = HandleNotify(controller, service, method_descriptor);
            } else {
                result = HandleRequest(controller, service, method_descriptor);
            }
        } else {
            result = HandleResponse(controller);
        }

        if (ERROR_BUSY == result) { // busy
            break;
        }

        handled = buffer_cur + controller_length;
    }

    // move
    buffer.len -= (handled - (int8_t*)buffer.base);
    if (buffer.len != 0 && handled != (int8_t*)buffer.base)
    {
        DLOG(INFO) << " connection: " << (int64_t)handle << " memmoved, buffer.base: " << (int64_t)buffer.base << " handled: " << (int64_t)handled << " len: " << buffer.len << " total: " << buffer.total;
        memmove(buffer.base, handled, buffer.len);
    }
    return result;
}

int32_t ChannelImpl::HandleRequest(Controller* controller, google::protobuf::Service* service, const google::protobuf::MethodDescriptor* method_descriptor)
{
    DLOG(INFO) << " connection: " << controller->connection_id() << " request: " << controller->proto().transmit_id();

    google::protobuf::Message* request = NULL;
    google::protobuf::Message* response = NULL;
    RemoteClosure* done = NULL;
    try {
        request = service->GetRequestPrototype(method_descriptor).New();
        response = service->GetResponsePrototype(method_descriptor).New();
        done = NewRemoteClosure();

        if (!request->ParseFromString(controller->proto().message())) {
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
    DLOG(INFO) << " connection: " << controller->connection_id() << " response: " << controller->proto().transmit_id() << " size: " << controller->proto().ByteSize();

    std::map<int64_t, Context>::iterator it = async_result_.find(controller->proto().transmit_id());
    if (it == async_result_.end() || it->second.controller->connection_id() != controller->connection_id()) {
        delete controller;
        return 0;
    }
    if (controller->Failed()) {
        it->second.controller->SetFailed(controller->ErrorText());
    } else {
        it->second.response->ParseFromString(controller->proto().message());
    }
    it->second.done->Run();
    async_result_.erase(it->first);
    transactions_[controller->connection_id()].erase(it->first);
    if (transactions_[controller->connection_id()].size() == 0) {
        transactions_.erase(controller->connection_id());
    }
    return 0;
}

int32_t ChannelImpl::HandleNotify(Controller* controller, google::protobuf::Service* service, const google::protobuf::MethodDescriptor* method)
{
    DLOG(INFO) << " connection: " << controller->connection_id() << " notify: " << controller->proto().transmit_id() << " size: " << controller->proto().ByteSize();

    google::protobuf::Message* request = NULL;
    try {
        request = service->GetRequestPrototype(method).New();
        if (!request->ParseFromString(controller->proto().message())) {
            delete controller;
            return 0;
        }
    } catch (std::bad_alloc) {
        delete request;
        delete controller;
        return ERROR_BUSY; // delay
    }

    service->CallMethod(method, controller, request, NULL, NULL);
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
        uv_tcp_nodelay((uv_tcp_t*)req->handle, 1);
        self->AddConnection(req->handle);
    } else if (req->handle == self->default_handle_) {
        self->default_handle_ = NULL;
    }
    free(req);
}

int64_t ChannelImpl::Connect(const char* host, int32_t port, bool as_default)
{
    uv_tcp_t* handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_connect_t* req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    if (NULL == handle || NULL == req) {
        free(handle);
        free(req);
        return ERROR_BUSY;
    }

    uv_tcp_init(loop_, handle);

    struct sockaddr_in address;
    uv_ip4_addr(host, port, &address);
    req->data = this;

    int result = uv_tcp_connect(req, handle, (struct sockaddr*)&address, OnConnect);
    if (result) {
        uv_close((uv_handle_t*)handle, OnClose);
        free(req);
        DLOG(WARNING) << uv_strerror(result);
        return ERROR_OTHER;
    }

    if (as_default) {
        default_handle_ = (uv_stream_t*)handle;
    }

    return (int64_t)(handle);
}

void ChannelImpl::OnAccept(uv_stream_t* handle, int32_t status)
{
    if (status) {
        return;
    }

    uv_tcp_t* peer_handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    if (NULL == peer_handle) {
        DLOG(WARNING) << " no more memory, denied";
        return;
    }

    uv_tcp_init(handle->loop, peer_handle);
    int result = uv_accept(handle, (uv_stream_t*)peer_handle);
    if (result) {
        DLOG(WARNING) << uv_strerror(result);
        uv_close((uv_handle_t*)peer_handle, OnClose);
        return;
    }

    uv_tcp_nodelay(peer_handle, 1);
    ChannelImpl* self = (ChannelImpl*)handle->data;
    self->AddConnection((uv_stream_t*)peer_handle);
}

int64_t ChannelImpl::Listen(const char* host, int32_t port, int32_t backlog)
{
    uv_tcp_t* handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    if (NULL == handle) {
        return ERROR_BUSY;
    }

    uv_tcp_init(loop_, handle);
    struct sockaddr_in address;
    int32_t result = 0;
    result = uv_ip4_addr(host, port, &address);
    if (result) {
        uv_close((uv_handle_t*)handle, OnClose);
        DLOG(WARNING) << uv_strerror(result);
        return ERROR_OTHER;
    }

    int32_t flags = 0;
    result = uv_tcp_bind(handle, (struct sockaddr*)&address, flags);
    if (result) {
        uv_close((uv_handle_t*)handle, OnClose);
        DLOG(WARNING) << uv_strerror(result);
        return ERROR_OTHER;
    }
    result = uv_listen((uv_stream_t*)handle, backlog, OnAccept);
    if (result) {
        uv_close((uv_handle_t*)handle, OnClose);
        DLOG(WARNING) << uv_strerror(result);
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
    DLOG(INFO) << " close connection: " << (int64_t)handle;

    uv_read_stop(handle);
    connected_handle_.erase((int64_t)(handle));
    buffer_.erase((int64_t)(handle));

    handle->data = this;
    uv_close((uv_handle_t*)handle, OnClose);

    /*
     * event
     */
    MockController controller;
    MockClosure closure;
    proto::ConnectionProto connection;
    connection.set_id((int64_t)handle);
    std::vector<proto::ConnectionEventService*>::iterator it = connection_event_service_.begin();
    for (; it != connection_event_service_.end(); it++) {
        (*it)->Disconnected(&controller, &connection, &connection, &closure);
    }
}

void ChannelImpl::AddConnection(uv_stream_t* handle)
{
    DLOG(INFO) << " new connection: " << (int64_t)handle;

    handle->data = this;
    connected_handle_[(int64_t)(handle)] = handle;
    uv_read_start(handle, OnAlloc, OnRead);

    /*
     * event
     */
    MockController controller;
    MockClosure closure;
    proto::ConnectionProto connection;
    connection.set_id((int64_t)handle);
    std::vector<proto::ConnectionEventService*>::iterator it = connection_event_service_.begin();
    for (; it != connection_event_service_.end(); it++) {
        (*it)->Connected(&controller, &connection, &connection, &closure);
    }
}

void ChannelImpl::OnAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    ChannelImpl* self = (ChannelImpl*)handle->data;
    Buffer& buffer = self->buffer_[(int64_t)(handle)];
    buffer.Expend(suggested_size);

    buf->base = (char*)buffer.base + buffer.len ;
    buf->len = buffer.total - buffer.len;
}

uv_stream_t* ChannelImpl::connected_handle(maid::Controller* controller)
{
    const proto::ConnectionProto& connection = controller->proto().GetExtension(proto::connection);

    uv_stream_t* handle = NULL;
    std::map<int64_t, uv_stream_t*>::const_iterator it = connected_handle_.find(connection.id());
    if (connected_handle_.end() != it) {
        handle = it->second;
    }
    return handle;
}

void ChannelImpl::set_default_connection_id(int64_t connection_id)
{
    std::map<int64_t, uv_stream_t*>::const_iterator it = connected_handle_.find(connection_id);
    if (connected_handle_.end() != it) {
        default_handle_ = it->second;
    }
}

int64_t ChannelImpl::default_connection_id()
{
    return (int64_t)default_handle_;
}
