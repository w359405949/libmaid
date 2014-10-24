using System;
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
        private int reconnect_ = -1;
        private string host_ = "";
        private int port_ = 0;
        private Socket connection_;
        private NetworkStream networkStream_;

        // double read buffer
        private MemoryStream readBuffer_;
        private MemoryStream readBufferBack_;

        private ControllerMetaSerializer serializer_;
        private Dictionary<string, MethodEventHandler> methods_;
        private byte[] buffer_;
        private int BUFFERSIZE = 4096;
        private uint transmit_id_ = 0;

        private IAsyncResult asyncRead_;
        private IAsyncResult asyncConnect_;

        public Channel()
        {
            readBuffer_ = new MemoryStream();
            readBufferBack_ = new MemoryStream();
            buffer_ = new byte[BUFFERSIZE];
            serializer_ = new ControllerMetaSerializer();
            methods_ = new Dictionary<string, MethodEventHandler>();

        }

        public void AddMethod(string fullMethodName, MethodEventHandler methodEventHandler)
        {
            methods_[fullMethodName] = methodEventHandler;
        }

        public bool Connected
        {
            get
            {
                return connection_ != null && connection_.Connected && networkStream_ != null;
            }
        }

        public bool Connecting
        {
            get
            {
                return asyncConnect_ != null && !asyncConnect_.IsCompleted;
            }
        }

        public void SetReconnect(int reconnect)
        {
            reconnect_ = reconnect;
        }

        public void Connect(string host, int port)
        {
            host_ = host;
            port_ = port;
            connection_ = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            asyncConnect_ = connection_.BeginConnect(host, port, null, null);
        }

        private void CheckConnection()
        {
            if (asyncConnect_ != null)
            {
                if (asyncConnect_.IsCompleted)
                {
                    if (connection_.Connected)
                    {
                        NewConnection();
                    }
                    else
                    {
                        CloseConnection();
                    }
                    asyncConnect_ = null;
                }
            }
            else if (!Connected)
            {
                Reconnect();
            }
        } 

        private void Reconnect()
        {
            if (Connecting)
            {
                return;
            }
            if (reconnect_ != 0)
            {
                Connect(host_, port_);
            }
            if (reconnect_ > 0)
            {
                reconnect_--;
            }
        }

        public void CloseConnection()
        {
            try
            {
                connection_.EndReceive(asyncRead_);
            }
            catch (Exception) { }
            try
            {
                connection_.EndConnect(asyncConnect_);
            }
            catch (Exception) { }
            try
            {
                connection_.Close();
            }
            catch (Exception) { }

            asyncConnect_ = null;
            asyncRead_ = null;
            connection_ = null;
            networkStream_ = null;
        }

        public void NewConnection()
        {
            networkStream_ = new NetworkStream(connection_);
            readBuffer_.Seek(0, SeekOrigin.Begin);
            readBuffer_.SetLength(0);
        }

        public long CallMethod(string fullServiceName, string methodName, byte[] message, bool notify=false)
        {
            Controller controller = new Controller();
            controller.meta = new ControllerMeta();
            controller.meta.message = message;
            controller.meta.full_service_name = fullServiceName;
            controller.meta.method_name = methodName;

            try
            {
                if (notify)
                {
                    SendNotify(controller);
                }
                else
                {
                    controller.meta.transmit_id = ++transmit_id_;
                    SendRequest(controller);
                }
            }
            catch (IOException)
            {
                return -1;
            }
            catch (ArgumentNullException)
            {
                return -1;
            }
            return controller.meta.transmit_id;
        }


        public long CallMethod(string fullMethodName, byte[] message, bool notify = false)
        {
            int last = fullMethodName.LastIndexOf(".");
            if (last < 0)
            {
                throw new Exception("invalid fullMethodName");
            }
            string fullServiceName = fullMethodName.Substring(0, last);
            string methodName = fullMethodName.Substring(last + 1, fullMethodName.Length - last - 1);

            return CallMethod(fullServiceName, methodName, message, notify);
        }


        public void Update()
        {
            CheckConnection();
            if (Connected)
            {
                Read();
                Handle();
            }
        }

        private void Read()
        {
            if (asyncRead_ != null && asyncRead_.IsCompleted)
            {
                int nread = connection_.EndReceive(asyncRead_);
                if (nread == 0)
                {
                    CloseConnection();
                    return;
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
