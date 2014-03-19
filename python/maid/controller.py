from google.protobuf.service import RpcController
from controller_pb2 import ControllerMeta

class Controller(RpcController):
    def __init__(self):
        self.meta_data = ControllerMeta()
        self.fd = 0
        self.sock = None

    def Reset(self):
        pass

    def Failed(self):
        return self.meta_data.failed

    def ErrorText(self):
        return self.meta_data.error_text

    def StartCancel(self):
        pass

    def SetFailed(self, reason):
        self.meta_data.failed = True
        self.meta_data.error_text = reason

    def IsCanceled(self):
        return self.meta_data.is_canceled

    def NotifyOnCancel(self, callback):
        pass
