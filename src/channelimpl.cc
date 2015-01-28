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

#include "helper.h"
#include "maid/controller.h"
#include "maid/controller.pb.h"
#include "maid/options.pb.h"
#include "maid/middleware.pb.h"
#include "maid/connection.pb.h"
#include "middleware/log_middleware.h"
#include "channelimpl.h"
#include "remoteclosure.h"
#include "define.h"
#include "buffer.h"
#include "context.h"
#include "mock.h"

using maid::ChannelImpl;

ChannelImpl::ChannelImpl(uv_loop_t* loop)
    :default_stream_(NULL),
     controller_max_size_(CONTROLLERMETA_MAX_SIZE),
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
    for (std::map< std::string, google::protobuf::Service*>::iterator it = service_.begin(); it != service_.end(); it++) {
        delete it->second;
    }
    service_.clear();

    for (std::vector<proto::Middleware*>::iterator it = middleware_.begin(); it != middleware_.end(); it++) {
        delete *it;
    }
    middleware_.clear();

    for (std::map<int64_t, uv_stream_t*>::iterator it = connected_handle_.begin(); it != connected_handle_.end(); it++) {
        it->second->data = NULL;
        RemoveConnection(it->second);
    }
    connected_handle_.clear();

    for (std::map<int64_t, uv_stream_t*>::iterator it = listen_handle_.begin(); it != listen_handle_.end(); it++) {
        it->second->data = NULL;
        uv_close((uv_handle_t*)it->second, OnCloseListen);
    }
    listen_handle_.clear();

    loop_ = NULL;
}

void ChannelImpl::AppendService(google::protobuf::Service* service)
{
    DLOG_IF(FATAL, service_.find(service->GetDescriptor()->full_name()) != service_.end()) << " same name service regist twice";
    service_[service->GetDescriptor()->full_name()] = service;
}

void ChannelImpl::AppendMiddleware(proto::Middleware* middleware)
{
    middleware_.push_back(middleware);
}

void ChannelImpl::CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* rpc_controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done)
{
    DCHECK(NULL != request);
    maid::Controller* controller = google::protobuf::down_cast<Controller*>(rpc_controller);
    proto::ControllerProto* controller_proto = controller->mutable_proto();
    controller_proto->set_full_service_name(method->service()->full_name());
    controller_proto->set_method_name(method->name());
    request->SerializeToString(controller_proto->mutable_message());

    if (!helper::ProtobufHelper::notify(*controller_proto)) {
        uint32_t transmit_id = ++transmit_id_max_; // TODO: check if used
        controller_proto->set_transmit_id(transmit_id);
        async_result_[transmit_id].controller = controller;
        async_result_[transmit_id].response = response;
        async_result_[transmit_id].done = done;

        const proto::ConnectionProto& connection_proto = controller_proto->GetExtension(proto::connection);
        if (connection_proto.has_id()) {
            transactions_[connection_proto.id()].insert(transmit_id);
        }
    } else {
        DLOG_IF(FATAL, NULL != done) << "notify method, done should be NULL";
        controller_proto = controller->release_proto();
    }

    SendRequest(controller_proto);
}

void ChannelImpl::SendRequest(proto::ControllerProto* controller_proto)
{
    controller_proto->set_stub(true);

    proto::ConnectionProto* connection_proto = controller_proto->MutableExtension(proto::connection);
    uv_stream_t* stream = connected_handle(*connection_proto, default_stream_);
    if (NULL == stream) {
        controller_proto->set_failed(true);
        controller_proto->set_error_text("not connected");
        HandleResponse(controller_proto);
        return;
    }
    connection_proto->set_id((int64_t)stream);

    if (controller_proto->has_transmit_id()) {
        DCHECK(!helper::ProtobufHelper::notify(*controller_proto));
        transactions_[connection_proto->id()].insert(controller_proto->transmit_id());
    }

    /*
     * middleware
     */
    std::vector<proto::Middleware*>::iterator it = middleware_.begin();
    for (; it != middleware_.end(); it++) {
        (*it)->ProcessRequest(MockController::default_instance(), controller_proto, controller_proto, MockClosure::default_instance());
    }

    int32_t controller_nl = htonl(controller_proto->ByteSize());
    int32_t send_buffer_size = sizeof(controller_nl) + controller_proto->ByteSize();
    int8_t* send_buffer = (int8_t*)malloc(send_buffer_size);
    if (NULL == send_buffer) {
        free(send_buffer);
        DLOG(ERROR) << " no more memory";
        controller_proto->set_failed(true);
        controller_proto->set_error_text("no more memory");
        HandleResponse(controller_proto);
        return;
    }

    memcpy(send_buffer, &controller_nl, sizeof(controller_nl));
    controller_proto->SerializeToArray(send_buffer + sizeof(controller_nl), controller_proto->ByteSize());

    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    if (NULL == req) {
        free(send_buffer);
        DLOG(ERROR) << " no more memory";
        controller_proto->set_failed(true);
        controller_proto->set_error_text("no more memory");
        HandleResponse(controller_proto);
        return;
    }

    req->data = controller_proto;

    uv_buf_t uv_buf;
    uv_buf.base = (char*)send_buffer;
    uv_buf.len = send_buffer_size;
    int error = uv_write(req, stream, &uv_buf, 1, AfterSendRequest);
    if (error) {
        free(req);
        free(send_buffer);
        DLOG(ERROR) << uv_strerror(error);
        controller_proto->set_failed(true);
        controller_proto->set_error_text(uv_strerror(error));
        HandleResponse(controller_proto);
        return;
    }

    sending_buffer_[req] = send_buffer;
}

void ChannelImpl::AfterSendRequest(uv_write_t* req, int32_t status)
{
    ChannelImpl* self = (ChannelImpl*)req->handle->data;
    proto::ControllerProto* controller_proto = (proto::ControllerProto*)req->data;
    delete self->sending_buffer_[req];
    self->sending_buffer_.erase(req);
    delete req;

    if (helper::ProtobufHelper::notify(*controller_proto)) {
        self->HandleResponse(controller_proto);
    } else if (status != 0) {
        controller_proto->set_failed(true);
        controller_proto->set_error_text(uv_strerror(status));
        self->HandleResponse(controller_proto);
    }
}

void ChannelImpl::AfterSendResponse(uv_write_t* req, int32_t status)
{
    ChannelImpl* self = (ChannelImpl*)req->handle->data;

    free(self->sending_buffer_[req]);
    self->sending_buffer_.erase(req);
    free(req);
}

void ChannelImpl::SendResponse(maid::proto::ControllerProto* controller_proto, const google::protobuf::Message* response)
{
    if (helper::ProtobufHelper::notify(*controller_proto)) {
        return;
    }

    controller_proto->set_stub(false);
    if (NULL != response) {
        try {
                response->SerializeToString(controller_proto->mutable_message());
        } catch (std::bad_alloc) {
            LOG(ERROR) << " no more memory";
            return;
        }
    }

    /*
     * middleware
     */
    std::vector<proto::Middleware*>::reverse_iterator it = middleware_.rbegin();
    for (; it != middleware_.rend(); it++) {
        (*it)->ProcessResponseStub(MockController::default_instance(), controller_proto, controller_proto, MockClosure::default_instance());
    }

    int32_t controller_nl = htonl(controller_proto->ByteSize());
    int32_t send_buffer_size = sizeof(controller_nl) + controller_proto->ByteSize();
    int8_t* send_buffer = (int8_t*)malloc(send_buffer_size);
    if (NULL == send_buffer) {
        LOG(ERROR) << " no more memory";
        return;
    }
    memcpy(send_buffer, &controller_nl, sizeof(controller_nl));
    controller_proto->SerializeToArray(send_buffer + sizeof(controller_nl), controller_proto->ByteSize());

    const proto::ConnectionProto& connection_proto = controller_proto->GetExtension(proto::connection);
    uv_stream_t* stream = connected_handle(connection_proto, NULL);
    if (NULL == stream) {
        free(send_buffer);
        LOG(ERROR) << " connection: " << connection_proto.id() << " not connected";
        return;
    }

    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    if (NULL == req) {
        free(send_buffer);
        LOG(ERROR) << " no more memory";
        return;
    }

    uv_buf_t uv_buf;
    uv_buf.len = send_buffer_size;
    uv_buf.base = (char*)send_buffer;
    int error = uv_write(req, stream, &uv_buf, 1, AfterSendResponse);
    if (error) {
        free(req);
        free(send_buffer);
        DLOG(ERROR) << uv_strerror(error);
        return;
    }

    sending_buffer_[req] = send_buffer;
}

void ChannelImpl::OnRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    ChannelImpl * self = (ChannelImpl*)stream->data;

    if (nread < 0) {
        self->RemoveConnection(stream);
        return;
    }

    /*
     * TODO: buf->base alloced by libuv (win).
     */
    self->buffer_[(int64_t)stream].end += nread;

    /*
     * libuv self schedular
     */
    //int32_t result = 0;
    //do {
    //    result = self->Handle((int64_t)stream);
    //} while(result != -2);

    /*
     *  timer schedular
     */
    uv_read_stop(stream);
    uv_timer_t& timer = self->timer_handle_[(int64_t)stream];
    uv_timer_start(&timer, OnTimer, 1, 1);

    /*
     * idle schedular
     */
    //uv_idle_t& idle = self->idle_handle_[(int64_t)stream];
    //uv_idle_start(&idle, OnIdle);
}

void ChannelImpl::OnIdle(uv_idle_t* idle)
{
    uv_stream_t* stream = (uv_stream_t*)idle->data;
    ChannelImpl* self = (ChannelImpl*)stream->data;
    int32_t result = self->Handle((int64_t)stream);

    // scheduler
    switch (result) {
        case ERROR_OUT_OF_SIZE:
            break;
        case ERROR_LACK_DATA:
            uv_idle_stop(idle);
            uv_read_start(stream, OnAlloc, OnRead);
            return;
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

void ChannelImpl::OnTimer(uv_timer_t* timer)
{
    uv_stream_t* stream = (uv_stream_t*)timer->data;
    ChannelImpl* self = (ChannelImpl*)stream->data;
    int32_t result = self->Handle((int64_t)stream);

    // scheduler
    switch (result) {
        case ERROR_OUT_OF_SIZE:
            break;
        case ERROR_LACK_DATA:
            uv_timer_stop(timer);
            uv_read_start(stream, OnAlloc, OnRead);
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

int32_t ChannelImpl::Handle(int64_t connection_id)
{
    Buffer& buffer = buffer_[connection_id];

    int32_t controller_size_nl = 0;
    if (buffer.start + sizeof(controller_size_nl) > buffer.end) {
        return ERROR_LACK_DATA;
    }

    memcpy(&controller_size_nl, buffer.start, sizeof(controller_size_nl));
    int32_t controller_size = ntohl(controller_size_nl);

    if (controller_size < 0 || controller_size > controller_max_size_) {
        buffer.start = buffer.end;
        DLOG(WARNING) << " connection: " << connection_id << " controller_size: " << controller_size << " out of max size: " << controller_max_size_;
        return ERROR_OUT_OF_SIZE;
    }

    if (buffer.start + sizeof(controller_size_nl) + controller_size > buffer.end) {
        return ERROR_LACK_DATA;
    }

    proto::ControllerProto* controller_proto = NULL;

    try {
        controller_proto = new proto::ControllerProto();
        if (!controller_proto->ParseFromArray(buffer.start + sizeof(controller_size_nl), controller_size)) {
            DLOG(WARNING) << " connection: " << connection_id << " parse ControllerProto failed";
            buffer.start += sizeof(controller_size_nl) + controller_size;
            return ERROR_PARSE_FAILED;
        }

        proto::ConnectionProto* connection_proto = controller_proto->MutableExtension(proto::connection);
        connection_proto->set_id(connection_id);
    } catch (std::bad_alloc) {
        delete controller_proto;
        return ERROR_BUSY;
    }
    buffer.start += sizeof(controller_size_nl) + controller_size;
    CHECK(buffer.start <= buffer.end);

    if (controller_proto->stub()) {
        return HandleRequest(controller_proto);
    } else {
        return HandleResponse(controller_proto);
    }

    return 0;
}

int32_t ChannelImpl::HandleRequest(proto::ControllerProto* controller_proto)
{
    /*
     * middleware
     */
    std::vector<proto::Middleware*>::reverse_iterator middleware_it = middleware_.rbegin();
    for (; middleware_it != middleware_.rend(); middleware_it++) {
        (*middleware_it)->ProcessRequestStub(MockController::default_instance(), controller_proto, controller_proto, MockClosure::default_instance());
    }

    std::map<std::string, google::protobuf::Service*>::iterator service_it;
    service_it = service_.find(controller_proto->full_service_name());
    if (service_.end() == service_it) {
        DLOG(WARNING) << " service: " << controller_proto->full_service_name() << " not exist";
        delete controller_proto;
        return ERROR_OTHER;
    }

    google::protobuf::Service* service = service_it->second;
    const google::protobuf::MethodDescriptor* method = NULL;
    method = service->GetDescriptor()->FindMethodByName(controller_proto->method_name());
    if (NULL == method) {
        DLOG(WARNING) << " service: " << controller_proto->full_service_name() << " method: " << controller_proto->method_name() << " not exist";
        delete controller_proto;
        return ERROR_OTHER;
    }

    maid::Controller* controller = NULL;
    google::protobuf::Message* request = NULL;
    google::protobuf::Message* response = NULL;
    RemoteClosure* done = NULL;

    try {
        done = NewRemoteClosure();
        controller = new Controller();

        request = service->GetRequestPrototype(method).New();
        if (!request->ParseFromString(controller_proto->message())) {
            delete controller;
            delete request;
            delete done;
            delete controller_proto;
            return ERROR_PARSE_FAILED;
        }

        response = service->GetResponsePrototype(method).New();

        done->set_controller(controller);
        done->set_request(request);
        done->set_response(response);

    } catch (std::bad_alloc) {
        delete controller;
        delete request;
        delete response;
        delete done;
        delete controller_proto;
        return ERROR_BUSY;
    }
    controller->set_allocated_proto(controller_proto);

    bool notify = helper::ProtobufHelper::notify(*controller_proto);

    // call
    service->CallMethod(method, controller, request, response, done);

    if (notify) {
        done->Reset();
    }

    return 0;
}

int32_t ChannelImpl::HandleResponse(proto::ControllerProto* controller_proto)
{
    if (helper::ProtobufHelper::notify(*controller_proto)) {
        delete controller_proto;
        return 0;
    }

    /*
     * middleware
     */
    std::vector<proto::Middleware*>::iterator middleware_it = middleware_.begin();
    for (; middleware_it != middleware_.end(); middleware_it++) {
        (*middleware_it)->ProcessResponse(MockController::default_instance(), controller_proto, controller_proto, MockClosure::default_instance());
    }

    const proto::ConnectionProto& connection_proto = controller_proto->GetExtension(proto::connection);

    if (connection_proto.has_id()) {
        std::set<int64_t>& transactions = transactions_[connection_proto.id()];
        if (transactions.end() == transactions.find(controller_proto->transmit_id())) {
            DLOG(WARNING) << "connection id miss";
            return 0;
        }
        transactions.erase(controller_proto->transmit_id());
        if (transactions.size() == 0) {
            transactions_.erase(connection_proto.id());
        }
    }

    std::map<int64_t, Context>::iterator it = async_result_.find(controller_proto->transmit_id());
    if (it == async_result_.end()) {
        DLOG(WARNING) << "commit id miss";
        return 0;
    }

    Context& context = it->second;
    if (!controller_proto->failed()) {
        context.response->ParseFromString(controller_proto->message());
    }
    context.controller->set_allocated_proto(controller_proto);
    context.done->Run();

    async_result_.erase(it);

    return 0;
}

maid::RemoteClosure* ChannelImpl::NewRemoteClosure()
{
    RemoteClosure* done = NULL;
    if (!remote_closure_pool_.empty()) {
        done = remote_closure_pool_.top();
        remote_closure_pool_.pop();
        done->Reset();
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
    } else if (req->handle == self->default_stream_) {
        self->default_stream_ = NULL;
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
    handle->data = this;

    int result = uv_tcp_connect(req, handle, (struct sockaddr*)&address, OnConnect);
    if (result) {
        DLOG(WARNING) << uv_strerror(result);
        free(req);
        uv_close((uv_handle_t*)handle, OnCloseConnection);
        return ERROR_OTHER;
    }

    if (as_default) {
        default_stream_ = (uv_stream_t*)handle;
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
        peer_handle->data = handle->data;
        uv_close((uv_handle_t*)peer_handle, OnCloseConnection);
        return;
    }

    uv_tcp_nodelay(peer_handle, 1);
    ChannelImpl* self = (ChannelImpl*)handle->data;
    self->AddConnection((uv_stream_t*)peer_handle);
}

void ChannelImpl::OnCloseListen(uv_handle_t* handle)
{
    ChannelImpl* self = (ChannelImpl*)handle->data;
    if (NULL != self) {
        self->listen_handle_.erase((int64_t)handle);
    }
    free(handle);
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
        uv_close((uv_handle_t*)handle, OnCloseListen);
        DLOG(WARNING) << uv_strerror(result);
        return ERROR_OTHER;
    }

    int32_t flags = 0;
    result = uv_tcp_bind(handle, (struct sockaddr*)&address, flags);
    if (result) {
        uv_close((uv_handle_t*)handle, OnCloseListen);
        DLOG(WARNING) << uv_strerror(result);
        return ERROR_OTHER;
    }
    result = uv_listen((uv_stream_t*)handle, backlog, OnAccept);
    if (result) {
        uv_close((uv_handle_t*)handle, OnCloseListen);
        DLOG(WARNING) << uv_strerror(result);
        return ERROR_OTHER;
    }

    handle->data = this;
    listen_handle_[(int64_t)handle] = (uv_stream_t*)handle;
    return (int64_t)handle;
}

void ChannelImpl::OnCloseConnection(uv_handle_t* handle)
{
    ChannelImpl* self = (ChannelImpl*)handle->data;
    if (NULL != self) {
        self->connected_handle_.erase((int64_t)handle);
    }
    free(handle);
}

void ChannelImpl::RemoveConnection(uv_stream_t* stream)
{
    uv_read_stop(stream);
    buffer_.erase((int64_t)(stream));

    uv_idle_stop(&idle_handle_[(int64_t)stream]);
    idle_handle_.erase((int64_t)stream);

    uv_timer_stop(&timer_handle_[(int64_t)stream]);
    timer_handle_.erase((int64_t)stream);

    if (default_stream_ == stream) {
        default_stream_ = NULL;
    }

    uv_close((uv_handle_t*)stream, OnCloseConnection);

    /*
     * middleware
     */
    proto::ConnectionProto connection_proto;
    connection_proto.set_id((int64_t)stream);
    std::vector<proto::Middleware*>::iterator middleware_it = middleware_.begin();
    for (; middleware_it != middleware_.end(); middleware_it++) {
        (*middleware_it)->Disconnected(MockController::default_instance(), &connection_proto, &connection_proto, MockClosure::default_instance());
    }
}

void ChannelImpl::AddConnection(uv_stream_t* stream)
{
    stream->data = this;
    connected_handle_[(int64_t)stream] = stream;
    uv_read_start(stream, OnAlloc, OnRead);

    uv_idle_t& idle = idle_handle_[(int64_t)stream];
    idle.data = stream;
    uv_idle_init(loop_, &idle);

    uv_timer_t& timer = timer_handle_[(int64_t)stream];
    timer.data = stream;
    uv_timer_init(loop_, &timer);

    /*
     * middleware
     */
    proto::ConnectionProto connection_proto;
    connection_proto.set_id((int64_t)stream);
    //connection.set_host();
    //connection.set_port();
    std::vector<proto::Middleware*>::iterator it = middleware_.begin();
    for (; it != middleware_.end(); it++) {
        (*it)->Connected(MockController::default_instance(), &connection_proto, &connection_proto, MockClosure::default_instance());
    }
}

void ChannelImpl::OnAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    ChannelImpl* self = (ChannelImpl*)handle->data;
    Buffer& buffer = self->buffer_[(int64_t)(handle)];

    buf->len = buffer.Expend(suggested_size);
    buf->base = (char*)buffer.end;
}

uv_stream_t* ChannelImpl::connected_handle(const maid::proto::ConnectionProto& connection_proto, uv_stream_t* stream)
{
    if (!connection_proto.has_id()) {
        return stream;
    }

    std::map<int64_t, uv_stream_t*>::iterator it = connected_handle_.find(connection_proto.id());
    if (connected_handle_.end() != it) {
        stream = it->second;
    }
    return stream;
}

void ChannelImpl::set_default_connection_id(int64_t connection_id)
{
    std::map<int64_t, uv_stream_t*>::const_iterator it = connected_handle_.find(connection_id);
    if (connected_handle_.end() != it) {
        default_stream_ = it->second;
    }
}

int64_t ChannelImpl::default_connection_id()
{
    return (int64_t)default_stream_;
}
