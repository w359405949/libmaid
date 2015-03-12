#pragma once

#include <google/protobuf/service.h>

namespace maid
{

class Controller;

struct Context
{
    Controller* controller;
    google::protobuf::Message* response;
    google::protobuf::Closure* done;

    void reset()
    {
        controller = NULL;
        response = NULL;
        done = NULL;
    }
};

} /* maid */
