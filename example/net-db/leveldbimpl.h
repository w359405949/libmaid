#ifndef NETDB_H
#define NETDB_H

#include <map>
#include <leveldb/db.h>

#include "db.pb.h"

class LevelDBImpl: public NetDBService
{
public:
    void Open(google::protobuf::RpcController* controller,
            const OpenRequest* request,
            OpenResponse* response,
            google::protobuf::Closure* done);


    void Get(google::protobuf::RpcController* controller,
            const GetRequest* request,
            GetResponse* response,
            google::protobuf::Closure* done);


    void Put(google::protobuf::RpcController* controller,
            const PutRequest* request,
            PutResponse* response,
            google::protobuf::Closure* done);


    void Delete(google::protobuf::RpcController* controller,
            const DeleteRequest* request,
            DeleteResponse* response,
            google::protobuf::Closure* done);


    void Close(google::protobuf::RpcController* controller,
            const CloseRequest* request,
            CloseResponse* response,
            google::protobuf::Closure* done);

public:
    LevelDBImpl(const std::string& base_path);

private:
    std::string base_path_;
    std::map<int32_t, std::string> dbs_;
    std::map<std::string, leveldb::DB*> opened_dbs_;
};

#endif /* NETDB_H */
