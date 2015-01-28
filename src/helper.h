#pragma once

namespace maid {

namespace proto {
class ControllerProto;
}
namespace helper {


class ProtobufHelper
{
public:
    static bool notify(const proto::ControllerProto& controller_proto, bool value=false);

};

} // helper
} // maid
