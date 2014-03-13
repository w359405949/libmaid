#include "closure.h"

using maid::channel::Channel;
using maid::channel::Context;
using maid::closure::RemoteClosure;

RemoteClosure::RemoteClosure(Channel* channel, Context* context)
    :channel_(channel),
    context_(context)
{
}

void RemoteClosure::Run()
{

}
