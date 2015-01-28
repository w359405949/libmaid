#include "gtest/gtest.h"
#include "remoteclosure.h"
#include "channelimpl.h"
#include "maid/controller.h"
#include "maid/controller.pb.h"
#include "test.pb.h"

class ChannelMock: public maid::ChannelImpl
{
public:
    virtual void SendResponse(maid::proto::ControllerProto* controller_proto, const google::protobuf::Message* response)
    {
    }

    void DeleteRemoteClosure(maid::RemoteClosure* done)
    {
    }
};

TEST(RemoteClosure, Construct)
{
    ChannelMock channel;
    maid::RemoteClosure remote_closure(&channel);

    ASSERT_EQ(NULL, remote_closure.request());
    ASSERT_EQ(NULL, remote_closure.controller());
    ASSERT_EQ(NULL, remote_closure.response());
    ASSERT_EQ(&channel, remote_closure.channel());
}

TEST(RemoteClosure, ResponseGetSet)
{
    ChannelMock channel;
    maid::RemoteClosure remote_closure(&channel);

    Response* response = new Response();
    remote_closure.set_response(response);
    ASSERT_EQ(response, remote_closure.response());
}

TEST(RemoteClosure, RequestGetSet)
{
    ChannelMock channel;
    maid::RemoteClosure remote_closure(&channel);

    Request* request = new Request();
    remote_closure.set_request(request);
    ASSERT_EQ(request, remote_closure.request());
}

TEST(RemoteClosure, ControllerGetSet)
{
    ChannelMock channel;
    maid::RemoteClosure remote_closure(&channel);

    maid::Controller* controller = new maid::Controller();
    remote_closure.set_controller(controller);
    ASSERT_EQ(controller, remote_closure.controller());
}

TEST(RemoteClosure, Run)
{
    ChannelMock channel;
    maid::RemoteClosure remote_closure(&channel);
    remote_closure.set_controller(new maid::Controller());
    remote_closure.Run();

    ASSERT_EQ(NULL, remote_closure.controller());
    ASSERT_EQ(NULL, remote_closure.request());
    ASSERT_EQ(NULL, remote_closure.response());
    ASSERT_EQ(&channel, remote_closure.channel());
}
