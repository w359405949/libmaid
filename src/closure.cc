#include <glog/logging.h>
#include "maid/controller.pb.h"

#include "closure.h"
#include "channel.h"
#include "controller.h"
#include "wire_format.h"
#include "helper.h"

namespace maid {

Closure::Closure()
{
}

Closure::~Closure()
{
}

void Closure::Run()
{
}

TcpClosure::TcpClosure(TcpChannel* channel, Controller* controller, google::protobuf::Message* request, google::protobuf::Message* response)
    :channel_(channel),
    send_buffer_(NULL),
    controller_(controller),
    request_(request),
    response_(response)
{
    req_.data = this;
}

void TcpClosure::Run()
{
    DLOG_IF(FATAL, send_buffer_ != NULL) << "TcpClosure::Run() call twice";
    if (controller_->IsCanceled() || helper::ProtobufHelper::notify(controller_->proto())) {
        gc_.data = this;
        uv_idle_init(uv_default_loop(), &gc_);
        uv_idle_start(&gc_, OnGc);
    }

    channel_->RemoveController(controller_);

    controller_->mutable_proto()->set_stub(false);
    if (!controller_->Failed()) {
        response_->SerializeToString(controller_->mutable_proto()->mutable_message());
    }
    send_buffer_ = WireFormat::Serializer(controller_->proto());

    uv_buf_t uv_buf;
    uv_buf.base = (char*)send_buffer_->data();
    uv_buf.len = send_buffer_->size();
    int error = uv_write(&req_, channel_->stream(), &uv_buf, 1, AfterSendResponse);
    if (error) {
        gc_.data = this;
        uv_idle_init(uv_default_loop(), &gc_);
        uv_idle_start(&gc_, OnGc);
        DLOG(ERROR) << uv_strerror(error);
        return;
    }
}

TcpClosure::~TcpClosure()
{
    delete send_buffer_;
    delete controller_;
    delete request_;
    delete response_;
}

void TcpClosure::OnGc(uv_idle_t* idle)
{
    uv_idle_stop(idle);
    TcpClosure* self = (TcpClosure*)idle->data;

    delete self;
}

void TcpClosure::AfterSendResponse(uv_write_t* handle, int32_t status)
{
    TcpClosure* self = (TcpClosure*)handle->data;
    delete self;
}

}
