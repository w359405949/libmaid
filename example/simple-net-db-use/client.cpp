#include <sys/types.h>
#include <stdio.h>

#include <ev.h>

#include "maid.h"
#include "db.pb.h"
#include "player.pb.h"

class OpenClosure : public maid::closure::SmartClosure
{
public:
    OpenClosure(struct ev_loop* loop, maid::channel::Channel* channel,
            maid::controller::Controller* controller)
        :SmartClosure(loop, channel, controller)
    {
    }

    void DoRun()
    {
        if(controller()->Failed()){
            printf("open: %s\n", controller()->ErrorText().c_str());
            return;
        }
        OpenRequest* request = (OpenRequest*)controller()->request();
        printf("open: %s\n", request->db_name().c_str());
    }

};


class PutClosure : public maid::closure::SmartClosure
{
public:
    PutClosure(struct ev_loop* loop, maid::channel::Channel* channel,
            maid::controller::Controller* controller)
        :SmartClosure(loop, channel, controller)
    {
    }

    void DoRun()
    {
        if(controller()->Failed()){
            printf("put: %s\n", controller()->ErrorText().c_str());
            return;
        }
        PutRequest* request = (PutRequest*)controller()->request();
        printf("put: %s\n", request->key().c_str());
    }

};


class GetClosure : public maid::closure::SmartClosure
{
public:
    GetClosure(struct ev_loop* loop, maid::channel::Channel* channel,
            maid::controller::Controller* controller)
        :SmartClosure(loop, channel, controller)
    {
    }

    void DoRun()
    {
        if(controller()->Failed()){
            printf("get: %s\n", controller()->ErrorText().c_str());
            return;
        }
        GetRequest* request = (GetRequest*)controller()->request();
        GetResponse* response = (GetResponse*)controller()->response();
        PlayerModel player;
        player.ParseFromString(response->value());
        printf("get: %s \n", request->key().c_str());
        player.PrintDebugString();
        ::fflush(stdout);
    }

};


class DeleteClosure : public maid::closure::SmartClosure
{
public:
    DeleteClosure(struct ev_loop* loop, maid::channel::Channel* channel,
            maid::controller::Controller* controller)
        :SmartClosure(loop, channel, controller)
    {
    }

    void DoRun()
    {
        if(controller()->Failed()){
            printf("delete: %s\n", controller()->ErrorText().c_str());
            return;
        }
        DeleteRequest* request = (DeleteRequest*)controller()->request();
        printf("delete: %s\n", request->key().c_str());
    }

};


class CloseClosure : public maid::closure::SmartClosure
{
public:
    CloseClosure(struct ev_loop* loop, maid::channel::Channel* channel,
            maid::controller::Controller* controller)
        :SmartClosure(loop, channel, controller)
    {
    }

    void DoRun()
    {
        if(controller()->Failed()){
            printf("close: %s\n", controller()->ErrorText().c_str());
            return;
        }
        printf("close: success\n");
    }

};

int main()
{
    maid::channel::Channel* channel = new maid::channel::Channel(EV_DEFAULT);
    NetDBService_Stub* db = new NetDBService_Stub(channel);
    int32_t fd = channel->Connect("0.0.0.0", 8888);

    // Open
    maid::controller::Controller* open_controller = new maid::controller::Controller(EV_DEFAULT);
    open_controller->set_fd(fd);
    OpenRequest* open_request = new OpenRequest();
    open_request->set_db_name("player");
    OpenResponse* open_response = new OpenResponse();
    db->Open(open_controller, open_request, open_response, new OpenClosure(EV_DEFAULT, channel, open_controller));

    // Put
    maid::controller::Controller* put_controller = new maid::controller::Controller(EV_DEFAULT);
    put_controller->set_fd(fd);
    PutRequest* put_request = new PutRequest();
    put_request->set_key("player_1");
    PlayerModel player;
    player.set_id(1);
    player.set_name("wucangqiong");
    player.set_sex("male");
    player.set_gold(12345);
    player.set_last_login_time(10000000);
    std::string value;
    player.SerializeToString(&value);
    put_request->set_value(value);
    PutResponse* put_response = new PutResponse();
    db->Put(put_controller, put_request, put_response, new PutClosure(EV_DEFAULT, channel, put_controller));

    // Get
    maid::controller::Controller* get_controller = new maid::controller::Controller(EV_DEFAULT);
    get_controller->set_fd(fd);
    GetRequest* get_request = new GetRequest();
    get_request->set_key("player_1");
    GetResponse* get_response = new GetResponse();
    db->Get(get_controller, get_request, get_response, new GetClosure(EV_DEFAULT, channel, get_controller));

    // Delete
    maid::controller::Controller* delete_controller = new maid::controller::Controller(EV_DEFAULT);
    delete_controller->set_fd(fd);
    DeleteRequest* delete_request = new DeleteRequest();
    delete_request->set_key("player_1");
    DeleteResponse* delete_response = new DeleteResponse();
    db->Delete(delete_controller, delete_request, delete_response, new DeleteClosure(EV_DEFAULT, channel, delete_controller));

    // Close
    maid::controller::Controller* close_controller = new maid::controller::Controller(EV_DEFAULT);
    close_controller->set_fd(fd);
    CloseRequest* close_request = new CloseRequest();
    CloseResponse* close_response = new CloseResponse();
    db->Close(close_controller, close_request, close_response, new CloseClosure(EV_DEFAULT, channel, close_controller));

    ev_run(EV_DEFAULT, 0);
}
