#include <google/protobuf/descriptor.h>

#include "maid/options.pb.h"
#include "maid/controller.pb.h"

#include "helper.h"

namespace maid {
namespace helper {

bool ProtobufHelper::notify(const maid::proto::ControllerProto& controller_proto, bool value)
{
    const google::protobuf::ServiceDescriptor* service = google::protobuf::DescriptorPool::generated_pool()->FindServiceByName(controller_proto.full_service_name());
    if (NULL == service) {
        return value;
    }

    const proto::MaidServiceOptions& service_options = service->options().GetExtension(proto::service_options);
    if (service_options.has_notify()) {
        value = service_options.notify();
    }

    const google::protobuf::MethodDescriptor* method = service->FindMethodByName(controller_proto.method_name());
    if (NULL == method) {
        return value;
    }

    const proto::MaidMethodOptions& method_options = method->options().GetExtension(proto::method_options);
    if (method_options.has_notify()) {
        value = method_options.notify();
    }
    return value;
}

} // helper
} // maid
