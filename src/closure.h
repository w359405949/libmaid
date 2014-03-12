
class BaseClosure : public google::protobuf::Closure
{
public:
    void set_method(google::protobuf::MethodDescriptor* method);
    void set_controller(google::protobuf::RpcController* controller);
    void set_request(google::protobuf::Message* request);
    void set_response(google::protobuf::Message* response);

    const google::protobuf::MethodDescriptor* get_method() const;
    const google::protobuf::RpcController* get_controller() const;
    const google::protobuf::Request* get_request() const;
    const google::protobuf::Response* get_response() const;

private:
    google::protobuf::MethodDescriptor * method_;
    google::protobuf::RpcController * controller_;
    google::protobuf::Message * request_;
    google::protobuf::Message * response_;
};

class RemoteClosure : public BaseClosure
{
public:
    void Run();

public:
    void CleanField();
};
