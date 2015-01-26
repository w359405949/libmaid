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
            channel.AddMethod<HelloRequest, HelloSerializer>("maid.example.HelloService.HelloNotify", service.HelloNotify);
            channel.AddMethod<HelloRequest, HelloSerializer, HelloResponse, HelloSerializer>("maid.example.HelloService.HelloRpc", service.HelloRpc);
            channel.ConnectedEvent += () =>
            {
                Console.WriteLine("连接上了");
            };

            channel.Connect("192.168.0.99", 5555);

            while (true)
            {
                channel.Update();

                HelloRequest request = new HelloRequest();
                request.message = "this message from protobuf-net";
                try
                {
                    channel.CallMethod("maid.example.HelloService.HelloNotify", request);
                    channel.CallMethod("maid.example.HelloService.HelloRpc", request);
                }
                catch (Exception ){ }
                if (channel.Connecting)
                {
                    Console.WriteLine("连接中");
                }
            }
        }
    }

    class HelloService
    {
        public void HelloRpc(Controller controller, HelloRequest request, HelloResponse response)
        {
            Console.WriteLine("request: " + request.message + " response: " + response.message);
        }

        public void HelloNotify(Controller controller, HelloRequest request)
        {
            Console.WriteLine("notify: " + request.message);
        }
    }
}
