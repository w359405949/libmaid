#pragma once

#include <string>
#include "buffer.h"

namespace maid {

namespace proto {
class ControllerProto;
} // proto

class WireFormat
{
public:
    static int32_t Deserializer(Buffer& buffer, proto::ControllerProto** controller_proto);
    static std::string* Serializer(const proto::ControllerProto& controller_proto);
};

} // maid
