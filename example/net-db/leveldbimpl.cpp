#include <assert.h>

#include <leveldb/db.h>

#include "maid.h"
#include "leveldbimpl.h"

LevelDBImpl::LevelDBImpl(const std::string& base_path)
    :base_path_(base_path)
{
}

void LevelDBImpl::Open(google::protobuf::RpcController* _controller,
        const OpenRequest* request,
        OpenResponse* response,
        google::protobuf::Closure* done)
{
    assert(("based libmaid, controller should use maid::controller::Controller", NULL != dynamic_cast<maid::controller::Controller*>(_controller)));

    maid::controller::Controller* controller = (maid::controller::Controller*)_controller;
    leveldb::DB* db = opened_dbs_[request->db_name()];
    if(NULL == db){
        leveldb::Options option;
        option.create_if_missing = true;
        leveldb::Status status = leveldb::DB::Open(option, base_path_ + request->db_name(), &db);
        if(!status.ok()){
            controller->SetFailed(status.ToString());
            done->Run();
            return;
        }
    }
    opened_dbs_[request->db_name()] = db;
    dbs_[controller->fd()] = request->db_name();
    done->Run();
}


void LevelDBImpl::Get(google::protobuf::RpcController* _controller,
        const GetRequest* request,
        GetResponse* response,
        google::protobuf::Closure* done)
{
    assert(("based libmaid, controller should use maid::controller::Controller", NULL != dynamic_cast<maid::controller::Controller*>(_controller)));

    maid::controller::Controller* controller = (maid::controller::Controller*)_controller;
    leveldb::DB* db = opened_dbs_[dbs_[controller->fd()]];
    if(NULL == db){
        controller->SetFailed("do not open any db");
        done->Run();
        return;
    }
    std::string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), request->key(), &value);
    if(!status.ok()){
        controller->SetFailed(status.ToString());
        done->Run();
        return;
    }
    response->set_value(value);
    done->Run();
}


void LevelDBImpl::Put(google::protobuf::RpcController* _controller,
        const PutRequest* request,
        PutResponse* response,
        google::protobuf::Closure* done)
{
    assert(("based libmaid, controller should use maid::controller::Controller", NULL != dynamic_cast<maid::controller::Controller*>(_controller)));

    maid::controller::Controller* controller = (maid::controller::Controller*)_controller;

    leveldb::DB* db = opened_dbs_[dbs_[controller->fd()]];
    if(NULL == db){
        controller->SetFailed("do not open any db");
        done->Run();
        return;
    }

    leveldb::Status status = db->Put(leveldb::WriteOptions(), request->key(), request->value());
    if(!status.ok()){
        controller->SetFailed(status.ToString());
        done->Run();
        return;
    }
    done->Run();
}


void LevelDBImpl::Delete(google::protobuf::RpcController* _controller,
        const DeleteRequest* request,
        DeleteResponse* response,
        google::protobuf::Closure* done)
{
    assert(("based libmaid, controller should use maid::controller::Controller", NULL != dynamic_cast<maid::controller::Controller*>(_controller)));

    maid::controller::Controller* controller = (maid::controller::Controller*)_controller;

    leveldb::DB* db = opened_dbs_[dbs_[controller->fd()]];
    if(NULL == db){
        controller->SetFailed("do not open any db");
        done->Run();
        return;
    }

    leveldb::Status status = db->Delete(leveldb::WriteOptions(), request->key());
    if(!status.ok()){
        controller->SetFailed(status.ToString());
        done->Run();
        return;
    }
    done->Run();
}


void LevelDBImpl::Close(google::protobuf::RpcController* _controller,
        const CloseRequest* request,
        CloseResponse* response,
        google::protobuf::Closure* done)
{
    assert(("based libmaid, controller should use maid::controller::Controller", NULL != dynamic_cast<maid::controller::Controller*>(_controller)));

    maid::controller::Controller* controller = (maid::controller::Controller*)_controller;
    dbs_[controller->fd()] = "";
    done->Run();
}
