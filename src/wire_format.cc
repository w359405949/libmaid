#include <glog/logging.h>
#include <arpa/inet.h>

#include "maid/controller.pb.h"
#include "wire_format.h"
#include "define.h"

namespace maid {

int32_t WireFormat::Deserializer(Buffer& buffer, proto::ControllerProto** controller_proto)
{
    int32_t controller_size_nl = 0;
    if (buffer.start + sizeof(controller_size_nl) > buffer.end) {
        return ERROR_LACK_DATA;
    }

    memcpy(&controller_size_nl, buffer.start, sizeof(controller_size_nl));
    int32_t controller_size = ntohl(controller_size_nl);

    if (controller_size < 0 || controller_size > CONTROLLERPROTO_MAX_SIZE) {
        buffer.start = buffer.end;
        return ERROR_OUT_OF_SIZE;
    }

    if (buffer.start + sizeof(controller_size_nl) + controller_size > buffer.end) {
        return ERROR_LACK_DATA;
    }

    try {
        (*controller_proto) = new proto::ControllerProto();
        if (!(*controller_proto)->ParseFromArray(buffer.start + sizeof(controller_size_nl), controller_size)) {
            buffer.start += sizeof(controller_size_nl) + controller_size;
            return ERROR_PARSE_FAILED;
        }
    } catch (std::bad_alloc) {
        delete (*controller_proto);
        (*controller_proto) = NULL;
        return ERROR_BUSY;
    }
    buffer.start += sizeof(controller_size_nl) + controller_size;
    CHECK(buffer.start <= buffer.end);
    return 0;
}

std::string* WireFormat::Serializer(const proto::ControllerProto& controller_proto)
{
    int32_t controller_nl = htonl(controller_proto.ByteSize());
    std::string* buffer = NULL;
    char controller_nl_buffer[sizeof(controller_nl)];
    memcpy(controller_nl_buffer, &controller_nl, sizeof(controller_nl));
    try {
        buffer = new std::string(controller_nl_buffer, sizeof(controller_nl));
        controller_proto.AppendToString(buffer);
    } catch (std::bad_alloc) {
        delete buffer;
        return NULL;
    }
    return buffer;
}

} // maid
