from maid.channel import Channel
from maid.controller import Controller
from hello_pb2 import HelloService_Stub
from hello_pb2 import HelloRequest

def main():
    channel = Channel()
    channel.connect("127.0.0.1", 8888, as_default=True)

    stub = HelloService_Stub(channel)
    controller = Controller()
    request = HelloRequest()
    print "meta:", controller.meta_data
    response = stub.Hello(controller, request, None)
    print "response:", response

if __name__ == "__main__":
    main()
