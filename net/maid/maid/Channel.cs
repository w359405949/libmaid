using System;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.IO;
using maid.proto;
using System.Collections.Generic;

namespace maid
{
    public class Channel
    {
        public delegate void MethodEventHandler(Controller controller);
        private Socket connection_;
        private NetworkStream networkStream_;

        // double read buffer
        private MemoryStream readBuffer_;
        private MemoryStream readBufferBack_;

        private ControllerMetaSerializer serializer_;
        private Dictionary<string, MethodEventHandler> methods_;
        private byte[] buffer_;
        private int BUFFERSIZE = 4096;
        private UInt32 transmit_id_ = 0;

        private event AsyncCallback afterConnect_;
        private IAsyncResult asyncRead_;
        private IAsyncResult asyncConnect_;

        public Channel()
        {
            readBuffer_ = new MemoryStream();
            readBufferBack_ = new MemoryStream();
            buffer_ = new byte[BUFFERSIZE];
            serializer_ = new ControllerMetaSerializer();
            methods_ = new Dictionary<string, MethodEventHandler>();
            afterConnect_ += AfterConnect;
        }

        private void AfterConnect(IAsyncResult asyncResult)
        {
            Socket connection = asyncResult.AsyncState as Socket;
            connection.EndConnect(asyncResult);

            if (connection.Connected)
            {
                NewConnection(connection);
            }
        }

        public void AddMethod(string fullMethodName, MethodEventHandler methodEventHandler)
        {
            methods_[fullMethodName] = methodEventHandler;
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
            Socket connection = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            asyncConnect_ = connection.BeginConnect(host, port, afterConnect_, connection);
            return asyncConnect_;
        }

        private void NewConnection(Socket connection)
        {
            connection_ = connection;
            networkStream_ = new NetworkStream(connection_);
        }

        public void CloseConnection()
        {
            if (connection_ != null)
            {
                try
               {
                    connection_.EndReceive(asyncRead_);
                }
                    catch (Exception) { }
                    asyncRead_ = null;

                    try
                    {
                        connection_.EndConnect(asyncConnect_);
                    }
                    catch (Exception) { }

                    asyncConnect_ = null;
                connection_.Close();
                connection_ = null;
                networkStream_ = null;
            }

            readBuffer_.Seek(0, SeekOrigin.Begin);
            readBuffer_.SetLength(0);
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


        public uint CallMethod(string fullMethodName, byte[] message, bool notify = false)
        {
            int last = fullMethodName.LastIndexOf(".");
            if (last < 0)
            {
                throw new Exception("invalid fullMethodName");
            }
            string fullServiceName = fullMethodName.Substring(0, last);
            string methodName = fullMethodName.Substring(last + 1, fullMethodName.Length - last - 1);

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
                    CloseConnection();
                }
                else
                {
                    readBuffer_.Seek(0, SeekOrigin.End);
                    readBuffer_.Write(buffer_, 0, nread);
                }
                asyncRead_ = null;
            }

            if (asyncRead_ == null)
            {
                asyncRead_ = connection_.BeginReceive(buffer_, 0, BUFFERSIZE, SocketFlags.None, null, null);
            }

            Handle();
        }


        protected void Handle()
        {
            if (readBuffer_.Length == 0)
            {
                return;
            }

            Controller controller = new Controller();
            controller.channel = this;
            try
            {
                readBuffer_.Seek(0, SeekOrigin.Begin);
                controller.meta = serializer_.DeserializeWithLengthPrefix(readBuffer_, null, typeof(ControllerMeta), ProtoBuf.PrefixStyle.Fixed32BigEndian, 0) as ControllerMeta;
            }
            catch (System.IO.EndOfStreamException)
            {
                return;
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

            // swap buffer
            readBufferBack_.Seek(0, SeekOrigin.Begin);
            readBufferBack_.SetLength(0);
            //readBuffer_.CopyTo(readBufferBack_);
            readBufferBack_.Write(readBuffer_.ToArray(), (int)readBuffer_.Position, (int)(readBuffer_.Length - readBuffer_.Position)); //WARN: may cause something bad
            MemoryStream ms = readBuffer_;
            readBuffer_ = readBufferBack_;
            readBufferBack_ = ms;
 
        }

        protected void HandleRequest(Controller controller)
        {
        }

        protected void HandleNotify(Controller controller)
        {
            string fullMethodName = controller.meta.full_service_name + "." + controller.meta.method_name;
            if (!methods_.ContainsKey(fullMethodName))
            {
                return;
            }
            methods_[fullMethodName](controller);
        }


        protected void HandleResponse(Controller controller)
        {
            string fullMethodName = controller.meta.full_service_name + "." + controller.meta.method_name;
            if (!methods_.ContainsKey(fullMethodName))
            {
                return;
            }
            methods_[fullMethodName](controller);
        }


        protected void SendNotify(Controller controller)
        {
            controller.meta.stub = true;
            controller.meta.notify = true;
            serializer_.SerializeWithLengthPrefix(networkStream_, controller.meta, typeof(ControllerMeta), ProtoBuf.PrefixStyle.Fixed32BigEndian, 0);
        }

        protected void SendRequest(Controller controller)
        {
            controller.meta.stub = true;
            controller.meta.notify = false;
            serializer_.SerializeWithLengthPrefix(networkStream_, controller.meta, typeof(ControllerMeta), ProtoBuf.PrefixStyle.Fixed32BigEndian, 0);
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
