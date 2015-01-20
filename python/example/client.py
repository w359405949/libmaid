from maid.channel import Channel
from maid.controller import Controller
from hello_pb2 import HelloService_Stub
from hello_pb2 import HelloRequest

def main():
    channel = Channel()
    channel.connect("127.0.0.1", 5555, as_default=True)

    stub = HelloService_Stub(channel)
    while True:
        controller = Controller()
        request = HelloRequest()
        request.message = "this message from pymaid"
        response = stub.Hello(controller, request, None)
        print "transmit_id:", controller.proto.transmit_id, "response:", response

if __name__ == "__main__":
    main()
