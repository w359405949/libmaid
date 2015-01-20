using System;
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
        public delegate void SendRequestFunc(Controller controller, Object request);
        public delegate void SendResponseFunc(Controller controller, Object response);
        public delegate void HandleRequestFunc(Controller controller);
        public delegate void HandleResponseFunc(Controller controller);
        public delegate void RpcCallFunc<RequestType, ResponseType>(Controller controller, RequestType request, ResponseType response);
        public delegate void NotifyCallFunc<RequestType>(Controller controller, RequestType request);

        private Dictionary<string, SendRequestFunc> sendRequestFunc_;
        private Dictionary<string, HandleRequestFunc> handleRequestFunc_;
        private Dictionary<string, HandleResponseFunc> handleResponseFunc_;

        private int reconnect_ = -1;
        private string host_ = "";
        private int port_ = 0;
        private Socket connection_;
        private NetworkStream networkStream_;

        // double read buffer
        private MemoryStream readBuffer_;
        private MemoryStream readBufferBack_;

        private ControllerProtoSerializer serializer_;

        private byte[] buffer_;
        private int BUFFERSIZE = 4096;

        private IAsyncResult asyncRead_;
        private IAsyncResult asyncConnect_;

        public Channel()
        {
            readBuffer_ = new MemoryStream();
            readBufferBack_ = new MemoryStream();
            buffer_ = new byte[BUFFERSIZE];
            serializer_ = new ControllerProtoSerializer();
            sendRequestFunc_ = new Dictionary<string, SendRequestFunc>();
            handleRequestFunc_ = new Dictionary<string, HandleRequestFunc>();
            handleResponseFunc_ = new Dictionary<string, HandleResponseFunc>();
        }

        public void AddMethod<RequestType, RequestSerializerType, ResponseType, ResponseSerializerType>(string fullMethodName, RpcCallFunc<RequestType, ResponseType> callback)
            where RequestType : class, ProtoBuf.IExtensible
            where RequestSerializerType : ProtoBuf.Meta.TypeModel
            where ResponseType : class, ProtoBuf.IExtensible
            where ResponseSerializerType : ProtoBuf.Meta.TypeModel
        {
            RpcCallFunc<RequestType, ResponseType> method = (controller, request, response) =>
            {
            };

            AddMethod<RequestType, RequestSerializerType, ResponseType, ResponseSerializerType>(fullMethodName, method, callback);
        }

        /*
         * package MyPackage;
         * serivce ServiceName
         * {
         *     rpc MethodName(RequestType) returns(ResponseType);
         * }
         *
         * fullMethodName = MyPackage + "." + ServiceName + "." + MethodName
         *
         */
        public void AddMethod<RequestType, RequestSerializerType, ResponseType, ResponseSerializerType>(string fullMethodName, RpcCallFunc<RequestType, ResponseType> method, RpcCallFunc<RequestType, ResponseType> callback)
            where RequestType : class, ProtoBuf.IExtensible
            where RequestSerializerType : ProtoBuf.Meta.TypeModel
            where ResponseType : class, ProtoBuf.IExtensible
            where ResponseSerializerType : ProtoBuf.Meta.TypeModel
        {
            Dictionary<UInt64, object> requests = new Dictionary<UInt64, object>();
            UInt64 transmit_id_ = 0;
            RequestSerializerType requestSerializer = Activator.CreateInstance<RequestSerializerType>();
            ResponseSerializerType responseSerializer = Activator.CreateInstance<ResponseSerializerType>();

            sendRequestFunc_[fullMethodName] = (controller, request) =>
            {
                controller.proto.transmit_id = ++transmit_id_;
                requests[controller.proto.transmit_id] = request as RequestType;

                using (MemoryStream stream = new MemoryStream())
                {
                    requestSerializer.Serialize(stream, request);
                    controller.proto.message = stream.ToArray();
                }
                controller.proto.stub = true;
                serializer_.SerializeWithLengthPrefix(networkStream_, controller.proto, typeof(ControllerProto), ProtoBuf.PrefixStyle.Fixed32BigEndian, 0);
            };

            handleResponseFunc_[fullMethodName] = (controller) =>
            {
                if (!requests.ContainsKey(controller.proto.transmit_id))
                {
                    return;
                }
                RequestType request = requests[controller.proto.transmit_id] as RequestType;

                ResponseType response = null;
                using (MemoryStream stream = new MemoryStream(controller.proto.message, 0, controller.proto.message.Length))
                {
                    response = responseSerializer.Deserialize(stream, null, typeof(ResponseType)) as ResponseType;
                }
                callback(controller, request as RequestType, response);

                requests.Remove(controller.proto.transmit_id);
            };

            handleRequestFunc_[fullMethodName] = (controller) =>
            {
                if (null == method)
                {
                    return;
                }
                RequestType request = null;
                ResponseType response = Activator.CreateInstance<ResponseType>();
                using (MemoryStream stream = new MemoryStream())
                {
                    request = requestSerializer.Deserialize(stream, null, typeof(RequestType)) as RequestType;
                }

                method(controller, request, response);

                using (MemoryStream stream = new MemoryStream())
                {
                    responseSerializer.Serialize(stream, response);
                    controller.proto.message = stream.ToArray();
                }
                controller.proto.stub = false;
                serializer_.SerializeWithLengthPrefix(networkStream_, controller.proto, typeof(ControllerProto), ProtoBuf.PrefixStyle.Fixed32BigEndian, 0);
            };
        }

        /*
         * package MyPackage;
         * serivce ServiceName
         * {
         *     rpc MethodName(RequestType) returns(ResponseType)
         *     {
         *          (maid.proto.method_options).notify = true;
         *     }
         * }
         *
         * fullMethodName = MyPackage + "." + ServiceName + "." + MethodName
         *
         */
        public void AddMethod<RequestType, RequestSerializerType>(string fullMethodName, NotifyCallFunc<RequestType> method)
            where RequestType : class, ProtoBuf.IExtensible
            where RequestSerializerType : ProtoBuf.Meta.TypeModel
        {
            RequestSerializerType requestSerializer = Activator.CreateInstance<RequestSerializerType>();

            sendRequestFunc_[fullMethodName] = (controller, request) =>
            {
                using (MemoryStream stream = new MemoryStream())
                {
                    requestSerializer.Serialize(stream, request);
                    controller.proto.message = stream.ToArray();
                }
                controller.proto.stub = true;
                serializer_.SerializeWithLengthPrefix(networkStream_, controller.proto, typeof(ControllerProto), ProtoBuf.PrefixStyle.Fixed32BigEndian, 0);
            };

            handleRequestFunc_[fullMethodName] = (controller) =>
            {
                RequestType request = null;
                using (MemoryStream stream = new MemoryStream(controller.proto.message, 0, controller.proto.message.Length))
                {
                    request = requestSerializer.Deserialize(stream, null, typeof(RequestType)) as RequestType;
                }
                method(controller, request);
            };
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

        public void CallMethod(string fullMethodName, object request)
        {
            if (!sendRequestFunc_.ContainsKey(fullMethodName))
            {
                throw new Exception("method:" + fullMethodName + " not registed");
            }

            int last = fullMethodName.LastIndexOf(".");
            if (last < 0)
            {
                throw new Exception("invalid fullMethodName");
            }

            string fullServiceName = fullMethodName.Substring(0, last);
            string methodName = fullMethodName.Substring(last + 1, fullMethodName.Length - last - 1);

            Controller controller = new Controller();
            controller.proto = new ControllerProto();
            controller.proto.full_service_name = fullServiceName;
            controller.proto.method_name = methodName;
            sendRequestFunc_[fullMethodName](controller, request);
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
                controller.proto = serializer_.DeserializeWithLengthPrefix(readBuffer_, null, typeof(ControllerProto), ProtoBuf.PrefixStyle.Fixed32BigEndian, 0) as ControllerProto;
            }
            catch (System.IO.EndOfStreamException)
            {
                return;
            }

            if (controller.proto.stub)
            {
                string fullMethodName = controller.proto.full_service_name + "." + controller.proto.method_name;
                if (!handleRequestFunc_.ContainsKey(fullMethodName))
                {
                    return;
                }
                handleRequestFunc_[fullMethodName](controller);
            }
            else
            {
                string fullMethodName = controller.proto.full_service_name + "." + controller.proto.method_name;
                if (!handleResponseFunc_.ContainsKey(fullMethodName))
                {
                    return;
                }
                handleResponseFunc_[fullMethodName](controller);
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
    }
}
