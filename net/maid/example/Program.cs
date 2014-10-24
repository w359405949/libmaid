using System;
using System.Collections.Generic;
using System.Threading;
using System.Net;
using maid;
using maid.example;

namespace Example
{
    class Program
    {
        static void Main(string[] args)
        {
            Channel channel = new Channel();
            HelloService service = new HelloService();
            channel.AddMethod("HelloService.Hello", service.Hello);
            channel.Connect("192.168.0.99", 8888);

            while (true)
            {
                HelloRequest request = new HelloRequest();
                request.message = "this message from protobuf-net";
                long transmitId = channel.CallMethod("HelloService.Hello", channel.Serialize(service.serializer_, request));
                if (transmitId == -1)
                {
                    if (channel.Connecting)
                    {
                        Console.WriteLine("断线了，重连中");
                    }
                }
                channel.Update();
            }
        }
    }

    class HelloService
    {
        public HelloSerializer serializer_;

        public HelloService()
        {
            serializer_ = new HelloSerializer();
        }

        public void Hello(Controller controller)
        {
            Console.WriteLine(controller.meta.transmit_id);
            if (controller.meta.stub)
            {
                HelloRequest request = controller.channel.Deserialize<HelloRequest>(serializer_, controller.meta.message);
                Console.WriteLine(request.message);
            }
            else
            {
                HelloResponse response = controller.channel.Deserialize<HelloResponse>(serializer_, controller.meta.message);
                Console.WriteLine(response.message);
            }
        }
    }
}
