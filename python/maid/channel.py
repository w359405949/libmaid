from google.protobuf.service import RpcChannel
from gevent.hub import get_hub
from gevent.pool import Pool
from gevent.event import AsyncResult
from gevent import socket
from gevent.queue import Queue

from controller_pb2 import ControllerMeta

from controller import Controller

class Channel(RpcChannel):

    def __init__(self, loop=None):
        self._header = "!HH"
        self._loop = loop or get_hub().loop
        self._services = {}
        self._socket = None
        self._pending_request = []
        self._tranmit_id = 0
        self._listen_watchers = {}

        self._send_queue = {}

    def append_service(self, service):
        self._services[service.DESCRIPTOR.full_name] = service

    def CallMethod(self, method, controller, request, response_class, done):
        if not instance(controller, Controller):
            raise Exception("controller should has type Controller")

        controller.meta_data.transmit_id = self._transmit_id
        controller.meta_data.stub = True

        # assume
        if self._transmit_id >= (1 << 63) -1:
            self._transmit_id = 0
        else:
            self._tranmit_id += 1
        controller.response_class = response_class
        controller.async_result = AsyncResult()

        if not self._send_queue.has_key(controller.sock):
            controller.SetFailed("did not connect")
            controller.async_result.set(None)
        else:
            self._send_queue[controller.sock].put(controller)
        return controller.async_result

    def connect(self, host, port):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
        while True:
            result = sock.connect_ex((host, port))
            if result == 0: # connected
                break
            elif result == socket.EINPROGRESS: # connection in progress
                continue
            else:
                raise Exception('%d: connection failed' % result)
        Greenlet.spawn(self._handle, sock)
        return sock

    def listen(self, host, port):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)

        # TODO:
        #sock.bind((host, port))
        accept_watcher = self._loop.io(sock.fileno(), EV_READ)
        aceept_watcher.start(sock, self._do_accept, sock)
        self._accept_watchers[sock] = accept_watcher

    def new_connection(self, sock):
        self._send_queue[sock] = Queue()

    def close_connection(self, sock):
        if not self._send_queue.has_key(sock):
            return
        sock.close()
        for controller in self._send_queue:
            if controller.meta_data.stub:
                controller.async_result.set(None)
        del self._send_queue[sock]

    def _do_accept(self, sock):
        try:
            client_socket, address = sock.accept()
        except _socket.error as err:
            if err.args[0] == EWOULDBLOCK:
                return
            raise
        Greenlet.spawn(self._handle, client_socket)

    def _handle(self, client_socket):
        self.new_connection(client_socket)
        while True:
            header_length = struct.calcsize(self._header)
            buffer = client_socket.read(header_length)
            controller_length, message_length = struct.unpack(self._header, buffer)
            controller_buffer = client_socket.read(controller_length)
            message_buffer = client_socket.read(message_length)

            controller = Controller()
            controller.sock = client_socket
            try:
                controller.meta_data.ParseFromString(controller_buffer)
            except DecodeError:
                break
            if controller.meta_data.stub: # request
                self._handle_request(controller, message_buffer)
            else:
                self._handle_response(controller, message_buffer)
        self.close_connection(client_socket)

    def _handle_request(self, controller, message_buffer):
        service = self._services.get(controller.meta_data.service_name, None)
        controller.meta_data.stub = False
        send_queue = self._send_queue[controller.sock]

        if service is None:
            controller.SetFailed("service not exist")
            send_queue.put(controller)
            return

        method = service.DESCRIPTOR.FindMethodByName(
                controller.meta_data.method_name)
        if method is None:
            controller.SetFailed("method not exist")
            send_queue.put(controller)
            return

        request_class = service.GetRequestClass(m)
        request = request_class()
        result = request.ParseFromString(message_buffer)
        if not result:
            return # can not resolved

        response = service.CallMethod(method, controller, request, None)
        controller.response = response
        send_queue.put(controller)

    def _handle_response(self, controller, message_buffer):
        controller = self._pending_request.get(controller.meta_data.transmit_id, None)
        if controller is None:
            return

        if controller.Failed:
            controller.async_result.set(None)
            return

        response = controller.response_class()
        result = response.ParseFromString(message_buffer)
        if not result:
            controller.SetFailed("parse failed")
            controller.async_result.set(None)
            return
        controller.async_result.set(response)
