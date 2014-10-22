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

            IAsyncResult result = channel.Connect("192.168.0.99", 8888);
            while (!result.IsCompleted)
            {
                Thread.Sleep(1000);
                Console.WriteLine("连接中");
            }
            if (!channel.Connected)
            {
                Console.WriteLine("没连上");
                Console.ReadKey();
            }
            Console.WriteLine("连上了");

            while (true)
            {
                //Thread.Sleep(10);
                HelloRequest request = new HelloRequest();
                request.message = "this message from protobuf-net";
                channel.CallMethod("HelloService.Hello", channel.Serialize(service.serializer_, request));
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
