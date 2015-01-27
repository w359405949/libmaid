using System;
using System.Net.Sockets;
using System.IO;
using maid.proto;
using System.Collections.Generic;
using ProtoBuf;

namespace maid
{
    public class Channel
    {
        private delegate void SendRequestFunc(Controller controller, Object request);
        private delegate void SendResponseFunc(Controller controller, Object response);
        private delegate void HandleRequestFunc(Controller controller);
        private delegate void HandleResponseFunc(Controller controller);

        public delegate void RpcCallFunc<RequestType, ResponseType>(Controller controller, RequestType request, ResponseType response);
        public delegate void NotifyCallFunc<RequestType>(Controller controller, RequestType request);
        public delegate void ConnectionFunc();

        private Dictionary<string, SendRequestFunc> sendRequestFunc_;
        private Dictionary<string, HandleRequestFunc> handleRequestFunc_;
        private Dictionary<string, HandleResponseFunc> handleResponseFunc_;
        private List<ConnectionFunc> ConnectedCallback_;
        private List<ConnectionFunc> DisconnectedCallback_;


        private int reconnect_ = -1;
        private string host_ = "";
        private int port_ = 0;
        private IAsyncResult asyncConnect_;
        private Socket connection_;


        // double write buffer
        private IAsyncResult asyncWrite_;
        private MemoryStream writeBuffer_; //
        private MemoryStream writeBufferPending_; //

        // double read buffer
        private IAsyncResult asyncRead_;
        private MemoryStream readBuffer_; //
        private MemoryStream readBufferBack_; //

        private ControllerProtoSerializer serializer_;
        private byte[] buffer_;

        public Channel()
        {
            readBuffer_ = new MemoryStream();
            readBufferBack_ = new MemoryStream();

            writeBuffer_ = new MemoryStream();
            writeBufferPending_ = new MemoryStream();

            serializer_ = new ControllerProtoSerializer();
            sendRequestFunc_ = new Dictionary<string, SendRequestFunc>();
            handleRequestFunc_ = new Dictionary<string, HandleRequestFunc>();
            handleResponseFunc_ = new Dictionary<string, HandleResponseFunc>();

            ConnectedCallback_ = new List<ConnectionFunc>();
            DisconnectedCallback_ = new List<ConnectionFunc>();
        }

        public void AddMethod<RequestType, RequestSerializerType, ResponseType, ResponseSerializerType>(string fullMethodName, RpcCallFunc<RequestType, ResponseType> callback)
            where RequestType : class, ProtoBuf.IExtensible
            where RequestSerializerType : ProtoBuf.Meta.TypeModel
            where ResponseType : class, ProtoBuf.IExtensible
            where ResponseSerializerType : ProtoBuf.Meta.TypeModel
        {
            AddMethod<RequestType, RequestSerializerType, ResponseType, ResponseSerializerType>(fullMethodName, null, callback);
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

                writeBufferPending_.Seek(0, SeekOrigin.End);
                serializer_.SerializeWithLengthPrefix(writeBufferPending_, controller.proto, typeof(ControllerProto), ProtoBuf.PrefixStyle.Fixed32BigEndian, 0);
            };

            if (null != callback)
            {
                handleResponseFunc_[fullMethodName] = (controller) =>
                {
                    if (!requests.ContainsKey(controller.proto.transmit_id))
                    {
                        return;
                    }
                    RequestType request = requests[controller.proto.transmit_id] as RequestType;
                    ResponseType response = null;

                    requests.Remove(controller.proto.transmit_id);
                    using (MemoryStream stream = new MemoryStream(controller.proto.message, 0, controller.proto.message.Length))
                    {
                        response = responseSerializer.Deserialize(stream, null, typeof(ResponseType)) as ResponseType;
                    }
                    callback(controller, request, response);
                };
            }

            if (method != null)
            {
                handleRequestFunc_[fullMethodName] = (controller) =>
                {
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

                    writeBufferPending_.Seek(0, SeekOrigin.End);
                    serializer_.SerializeWithLengthPrefix(writeBufferPending_, controller.proto, typeof(ControllerProto), ProtoBuf.PrefixStyle.Fixed32BigEndian, 0);
                };
            }

            ConnectedCallback_.Add(() =>
            {
                requests.Clear();
            });

            DisconnectedCallback_.Add(() =>
            {
                requests.Clear();
            });

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
                serializer_.SerializeWithLengthPrefix(writeBufferPending_, controller.proto, typeof(ControllerProto), ProtoBuf.PrefixStyle.Fixed32BigEndian, 0);
            };

            if (method != null)
            {
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
        }

        public void CallMethod(string fullMethodName, object request)
        {
            if (!sendRequestFunc_.ContainsKey(fullMethodName))
            {
                throw new Exception("method: " + fullMethodName + " not registed");
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

        public bool Connected
        {
            get
            {
                return connection_ != null && connection_.Connected;
            }
        }

        public bool Connecting
        {
            get
            {
                return asyncConnect_ != null && reconnect_ != 0;
            }
        }

        public int reconnect
        {
            get
            {
                return reconnect_;
            }

            set
            {
                reconnect_ = value;
            }
        }

        public void Connect(string host, int port)
        {
            if (Connected)
            {
                CloseConnection();
            }

            host_ = host;
            port_ = port;
            Socket connection = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            asyncConnect_ = connection.BeginConnect(host, port, null, connection);
        }

        private void Reconnect()
        {
            if (asyncConnect_ != null)
            {
                Socket connection = asyncConnect_.AsyncState as Socket;
                try
                {
                    connection.EndConnect(asyncConnect_);
                    if (connection.Connected)
                    {
                        NewConnection(connection);
                        return;
                    }
                }
                catch (Exception)
                {
                }
                finally
                {
                    asyncConnect_ = null;
                }
            }

            if (asyncConnect_ == null)
            {
                if (reconnect_ != 0)
                {
                    Connect(host_, port_);
                }
                if (reconnect_ > 0)
                {
                    reconnect_--;
                }
            }
        }

        public void NewConnection(Socket connection)
        {
            asyncRead_ = null;
            readBuffer_.Seek(0, SeekOrigin.Begin);
            readBuffer_.SetLength(0);
            readBufferBack_.Seek(0, SeekOrigin.Begin);
            readBufferBack_.SetLength(0);

            asyncWrite_ = null;
            writeBuffer_.Seek(0, SeekOrigin.Begin);
            writeBuffer_.SetLength(0);
            writeBufferPending_.Seek(0, SeekOrigin.Begin);
            writeBufferPending_.SetLength(0);

            connection.NoDelay = true;
            buffer_ = new byte[connection.ReceiveBufferSize];
            connection_ = connection;

            foreach (ConnectionFunc callback in ConnectedCallback_)
            {
                callback();
            }
        }

        public void CloseConnection()
        {
            try
            {
                connection_.EndConnect(asyncConnect_);
            }
            catch (Exception) { }
            try
            {
                connection_.EndReceive(asyncRead_);
            }
            catch (Exception) { }
            try
            {
                connection_.EndSend(asyncWrite_);
            }
            catch (Exception) { }
            try
            {
                connection_.Close();
            }
            catch (Exception) { }

            asyncConnect_ = null;
            asyncWrite_ = null;
            asyncRead_ = null;
            connection_ = null;

            foreach (ConnectionFunc callback in DisconnectedCallback_)
            {
                callback();
            }
        }


        public void Update()
        {
            if (!Connected)
            {
                Reconnect();
                return;
            }
            try
            {
                Read();
                Handle();
                Write();
            }
            catch (SocketException)
            {
                CloseConnection();
                return;
            }
        }

        private void Write()
        {
            if (asyncWrite_ != null && asyncWrite_.IsCompleted)
            {
                int nwrite = connection_.EndSend(asyncWrite_);

                if (nwrite < writeBuffer_.Length)
                {
                    byte[] data = writeBuffer_.ToArray();
                    int remain = data.Length - nwrite;
                    asyncWrite_ = connection_.BeginSend(data, nwrite, remain, SocketFlags.None, null, null);
                }
                else
                {
                    writeBuffer_.Seek(0, SeekOrigin.Begin);
                    writeBuffer_.SetLength(0);
                    asyncWrite_ = null;
                }
            }

            if (asyncWrite_ == null)
            {

                MemoryStream ms = writeBuffer_;
                writeBuffer_ = writeBufferPending_;
                writeBufferPending_ = ms;

                if (writeBuffer_.Length > 0)
                {
                    byte[] data = writeBuffer_.ToArray();
                    asyncWrite_ = connection_.BeginSend(data, 0, data.Length, SocketFlags.None, null, null);
                }
            }
        }

        private void Read()
        {
            if (asyncRead_ != null && asyncRead_.IsCompleted)
            {
                int nread = connection_.EndReceive(asyncRead_);
                if (nread < 0)
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
                asyncRead_ = connection_.BeginReceive(buffer_, 0, buffer_.Length, SocketFlags.None, null, null);
            }
        }

        protected void Handle()
        {
            readBuffer_.Seek(0, SeekOrigin.Begin);

            if (readBuffer_.Length < 4)
            {
                return;
            }

            int fieldNumber;
            int len = ProtoReader.ReadLengthPrefix(readBuffer_, false, PrefixStyle.Fixed32BigEndian, out fieldNumber);
            if (len > readBuffer_.Length - 4)
            {
                readBuffer_.Seek(0, SeekOrigin.Begin);
                return;
            }

            Controller controller = new Controller();
            controller.channel = this;
            controller.proto = serializer_.Deserialize(readBuffer_, null, typeof(ControllerProto), len) as ControllerProto;

            // swap buffer
            readBufferBack_.Seek(0, SeekOrigin.Begin);
            readBufferBack_.SetLength(0);
            readBufferBack_.Write(readBuffer_.ToArray(), (int)readBuffer_.Position, (int)(readBuffer_.Length - readBuffer_.Position)); //WARN: may cause something bad
            MemoryStream ms = readBuffer_;
            readBuffer_ = readBufferBack_;
            readBufferBack_ = ms;

            if (controller.proto.stub)
            {
                string fullMethodName = controller.proto.full_service_name + "." + controller.proto.method_name;
                if (handleRequestFunc_.ContainsKey(fullMethodName))
                {
                    handleRequestFunc_[fullMethodName](controller);
                }
            }
            else
            {
                string fullMethodName = controller.proto.full_service_name + "." + controller.proto.method_name;
                if (handleResponseFunc_.ContainsKey(fullMethodName))
                {
                    handleResponseFunc_[fullMethodName](controller);
                }
            }
        }
    }
}
