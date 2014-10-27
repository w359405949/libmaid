#ifndef _MAID_CONTEXT_H_
#define _MAID_CONTEXT_H_

#include <google/protobuf/service.h>

namespace maid
{

class Controller;

struct Context
{
    Controller* controller;
    google::protobuf::Message* response;
    google::protobuf::Closure* done;
};

} /* maid */

#endif /* _MAID_CONTEXT_H_ */
