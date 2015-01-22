from google.protobuf.service import RpcController
from controller_pb2 import ControllerProto

class Controller(RpcController):
    def __init__(self):
        self.proto = ControllerProto()
        self.fd = 0
        self.sock = None

    def Reset(self):
        pass

    def Failed(self):
        return self.proto.failed

    def ErrorText(self):
        return self.proto.error_text

    def StartCancel(self):
        pass

    def SetFailed(self, reason):
        self.proto.failed = True
        self.proto.error_text = reason

    def IsCanceled(self):
        return self.proto.is_canceled

    def NotifyOnCancel(self, callback):
        pass
