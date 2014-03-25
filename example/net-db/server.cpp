#include <ev.h>
#include "leveldbimpl.h"
#include "maid.h"

int main()
{
    maid::channel::Channel* channel = new maid::channel::Channel(EV_DEFAULT);
    channel->AppendService(new LevelDBImpl("."));
    channel->Listen("0.0.0.0", 8888);

    ev_run(EV_DEFAULT, 0);
}
