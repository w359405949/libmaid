using System;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.IO;
using maid.proto;
using System.Collections.Generic;
using System.Reflection;
namespace maid
{
    public class Channel
    {
        private Socket connection_;
        private MemoryStream readBuffer_;
        private ControllerSerializer serializer_;
        private Dictionary<string, object> services_;
        private byte[] buffer_;
        private int BUFFERSIZE = 4096;
        private UInt32 transmit_id_ = 0;

        private IAsyncResult asyncRead_;

        public Channel()
        {
            readBuffer_ = new MemoryStream();
            buffer_ = new byte[BUFFERSIZE];
            serializer_ = new ControllerSerializer();
            services_ = new Dictionary<string, object>();
        }

        public void AddService(string serviceName, Object service)
        {
            services_[serviceName] = service;
        }

        public bool Connected  
        {
            get
            {
                return connection_ != null && connection_.Connected;
            } 
        }

        public IAsyncResult Connect(string host, int port)
        {
            connection_ = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            return connection_.BeginConnect(host, port, null, null);
        }

        public void Close()
        {
            if (connection_ != null)
            {
                connection_.Close();
            }
            connection_ = null;
            readBuffer_.Seek(readBuffer_.Length, SeekOrigin.Begin);
        }

        public uint CallMethod(string fullServiceName, string methodName, byte[] message, bool notify=false)
        {
            Controller controller = new Controller();
            controller.meta = new ControllerMeta();
            controller.meta.message = message;
            controller.meta.full_service_name = fullServiceName;
            controller.meta.method_name = methodName;

            if (notify)
            {
                SendNotify(controller);
            }
            else
            {
                controller.meta.transmit_id = ++transmit_id_;
                SendRequest(controller);
            }
            return controller.meta.transmit_id;
        }

        public void Update()
        {
            if (!Connected)
            {
                return;
            }   
            Read();
        }

        private void Read()
        {
            if (asyncRead_ != null && asyncRead_.IsCompleted)
            {
                int nread = connection_.EndReceive(asyncRead_);
                if (nread == 0)
                {
                    Close();
                }
                else
                {
                    readBuffer_.Write(buffer_, 0, nread);
                }

                asyncRead_ = null;
            }

            if (Connected && asyncRead_ == null)
            {
                asyncRead_ = connection_.BeginReceive(buffer_, 0, BUFFERSIZE, 0, null, null);
            }

            Handle();
        }

        protected void Handle()
        {
            byte[] buffer = readBuffer_.ToArray();
            if (buffer.Length < 4)
            {
                return;
            }
            int bodyLength = BitConverter.ToInt32(buffer, 0);
            bodyLength = IPAddress.NetworkToHostOrder(bodyLength);
            if (buffer.Length < 4 + bodyLength)
            {
                return;
            }

            Controller controller = new Controller();
            controller.channel = this;

            using(MemoryStream stream = new MemoryStream(buffer, 4, bodyLength))
            {
                controller.meta = serializer_.Deserialize(stream, null, typeof(ControllerMeta)) as ControllerMeta;
            }

            if (controller.meta.stub)
            {
                if (controller.meta.notify) 
                {
                    HandleNotify(controller);
                }
                else
                {
                    HandleRequest(controller);
                }
            }
            else
            {
                HandleResponse(controller);
            }
            readBuffer_.Seek(4 + bodyLength, SeekOrigin.Begin);
            readBuffer_.Position = 0;
        }

        protected void HandleRequest(Controller controller)
        {
        }

        protected void HandleNotify(Controller controller)
        {
            if (!services_.ContainsKey(controller.meta.full_service_name))
            {
                return;
            }
            Object service = services_[controller.meta.full_service_name];
            MethodInfo method = service.GetType().GetMethod(controller.meta.method_name);
            if (method == null)
            {
                return;
            }
            object[] parameters = {controller};
            method.Invoke(service, parameters);
        }


        protected void HandleResponse(Controller controller)
        {
            if (!services_.ContainsKey(controller.meta.full_service_name))
            {
                return;
            }
            Object service = services_[controller.meta.full_service_name];
            
            MethodInfo method = service.GetType().GetMethod(controller.meta.method_name);
            if (method == null)
            {
                return;
            }
            object[] parameters = { controller };
            method.Invoke(service, parameters);
        }


        protected void SendNotify(Controller controller)
        {
            controller.meta.stub = true;
            controller.meta.notify = true;

            using (MemoryStream stream = new MemoryStream())
            {
                serializer_.Serialize(stream, controller.meta);
                byte[] message = stream.ToArray();
                Int32 controllerLengthNet = IPAddress.HostToNetworkOrder(message.Length);
                connection_.Send(BitConverter.GetBytes(controllerLengthNet));
                connection_.Send(message, 0, (int)stream.Length, 0);
            }
        }

        protected void SendRequest(Controller controller)
        {
            controller.meta.stub = true;
            controller.meta.notify = false;

            using (MemoryStream stream = new MemoryStream())
            {
                serializer_.Serialize(stream, controller.meta);
                byte[] message = stream.ToArray();
                Int32 controllerLengthNet = IPAddress.HostToNetworkOrder(message.Length);
                connection_.Send(BitConverter.GetBytes(controllerLengthNet));
                int length = connection_.Send(stream.ToArray(), 0, (int)stream.Length, 0);
            }
        }

        public static T Deserialize<T>(ProtoBuf.Meta.TypeModel serializer, byte[] bytes, int index, int count) where T:class
        {
            T obj = null;
            using (MemoryStream stream = new MemoryStream(bytes, index, count))
            {
                obj = serializer.Deserialize(stream, null, typeof(T)) as T;
            }
            return obj;
        }

        public T Deserialize<T>(ProtoBuf.Meta.TypeModel serializer, byte[] bytes) where T : class
        {
            T obj = null;
            using (MemoryStream stream = new MemoryStream(bytes, 0, bytes.Length))
            {
                obj = serializer.Deserialize(stream, null, typeof(T)) as T;
            }
            return obj;
        }

        public byte[] Serialize(ProtoBuf.Meta.TypeModel serializer, object message)
        {
            byte[] bytes = null;
            using (MemoryStream stream = new MemoryStream())
            {
                serializer.Serialize(stream, message);
                bytes = stream.ToArray();
            }
            return bytes;
        }
    }
}
