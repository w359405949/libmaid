# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: controller.proto

import sys
_b=sys.version_info[0]<3 and (lambda x:x) or (lambda x:x.encode('latin1'))
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
from google.protobuf import descriptor_pb2
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='controller.proto',
  package='maid.proto',
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> bd6016103fecc4c9bae8b8be8002b50d7f3e8f55
  serialized_pb=_b('\n\x10\x63ontroller.proto\x12\nmaid.proto\"\xc9\x01\n\x0f\x43ontrollerProto\x12\x13\n\x0bmethod_name\x18\x02 \x01(\t\x12\x13\n\x0btransmit_id\x18\x03 \x01(\r\x12\x0c\n\x04stub\x18\x04 \x01(\x08\x12\x13\n\x0bis_canceled\x18\x05 \x01(\x08\x12\x0e\n\x06\x66\x61iled\x18\x06 \x01(\x08\x12\x12\n\nerror_text\x18\x07 \x01(\t\x12\x0e\n\x06notify\x18\t \x01(\x08\x12\x0f\n\x07message\x18\n \x01(\x0c\x12\x19\n\x11\x66ull_service_name\x18\x0b \x01(\t*\t\x08\xe8\x07\x10\x80\x80\x80\x80\x02')
)
=======
  serialized_pb=_b('\n\x10\x63ontroller.proto\x12\nmaid.proto\x1a google/protobuf/descriptor.proto\"\xc8\x01\n\x0e\x43ontrollerMeta\x12\x13\n\x0bmethod_name\x18\x02 \x01(\t\x12\x13\n\x0btransmit_id\x18\x03 \x01(\r\x12\x0c\n\x04stub\x18\x04 \x01(\x08\x12\x13\n\x0bis_canceled\x18\x05 \x01(\x08\x12\x0e\n\x06\x66\x61iled\x18\x06 \x01(\x08\x12\x12\n\nerror_text\x18\x07 \x01(\t\x12\x0e\n\x06notify\x18\t \x01(\x08\x12\x0f\n\x07message\x18\n \x01(\x0c\x12\x19\n\x11\x66ull_service_name\x18\x0b \x01(\t*\t\x08\xe8\x07\x10\x80\x80\x80\x80\x02:6\n\x06notify\x12\x1e.google.protobuf.MethodOptions\x18\x91N \x01(\x08:\x05\x66\x61lse')
  ,
  dependencies=[google.protobuf.descriptor_pb2.DESCRIPTOR,])
>>>>>>> a665b01... backup
=======
  serialized_pb=_b('\n\x10\x63ontroller.proto\x12\nmaid.proto\"\xc8\x01\n\x0e\x43ontrollerMeta\x12\x13\n\x0bmethod_name\x18\x02 \x01(\t\x12\x13\n\x0btransmit_id\x18\x03 \x01(\r\x12\x0c\n\x04stub\x18\x04 \x01(\x08\x12\x13\n\x0bis_canceled\x18\x05 \x01(\x08\x12\x0e\n\x06\x66\x61iled\x18\x06 \x01(\x08\x12\x12\n\nerror_text\x18\x07 \x01(\t\x12\x0e\n\x06notify\x18\t \x01(\x08\x12\x0f\n\x07message\x18\n \x01(\x0c\x12\x19\n\x11\x66ull_service_name\x18\x0b \x01(\t*\t\x08\xe8\x07\x10\x80\x80\x80\x80\x02')
=======
  serialized_pb=_b('\n\x10\x63ontroller.proto\x12\nmaid.proto\"\xc9\x01\n\x0f\x43ontrollerProto\x12\x13\n\x0bmethod_name\x18\x02 \x01(\t\x12\x13\n\x0btransmit_id\x18\x03 \x01(\r\x12\x0c\n\x04stub\x18\x04 \x01(\x08\x12\x13\n\x0bis_canceled\x18\x05 \x01(\x08\x12\x0e\n\x06\x66\x61iled\x18\x06 \x01(\x08\x12\x12\n\nerror_text\x18\x07 \x01(\t\x12\x0e\n\x06notify\x18\t \x01(\x08\x12\x0f\n\x07message\x18\n \x01(\x0c\x12\x19\n\x11\x66ull_service_name\x18\x0b \x01(\t*\t\x08\xe8\x07\x10\x80\x80\x80\x80\x02')
>>>>>>> 4657d14... update
)
>>>>>>> ffae88c... backup
_sym_db.RegisterFileDescriptor(DESCRIPTOR)




_CONTROLLERPROTO = _descriptor.Descriptor(
  name='ControllerProto',
  full_name='maid.proto.ControllerProto',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='method_name', full_name='maid.proto.ControllerProto.method_name', index=0,
      number=2, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='transmit_id', full_name='maid.proto.ControllerProto.transmit_id', index=1,
      number=3, type=13, cpp_type=3, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='stub', full_name='maid.proto.ControllerProto.stub', index=2,
      number=4, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='is_canceled', full_name='maid.proto.ControllerProto.is_canceled', index=3,
      number=5, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='failed', full_name='maid.proto.ControllerProto.failed', index=4,
      number=6, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='error_text', full_name='maid.proto.ControllerProto.error_text', index=5,
      number=7, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='notify', full_name='maid.proto.ControllerProto.notify', index=6,
      number=9, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='message', full_name='maid.proto.ControllerProto.message', index=7,
      number=10, type=12, cpp_type=9, label=1,
      has_default_value=False, default_value=_b(""),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='full_service_name', full_name='maid.proto.ControllerProto.full_service_name', index=8,
      number=11, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  options=None,
  is_extendable=True,
  extension_ranges=[(1000, 536870912), ],
  oneofs=[
  ],
<<<<<<< HEAD
<<<<<<< HEAD
  serialized_start=33,
  serialized_end=234,
<<<<<<< HEAD
)

DESCRIPTOR.message_types_by_name['ControllerProto'] = _CONTROLLERPROTO
=======
  serialized_start=67,
  serialized_end=267,
)

DESCRIPTOR.message_types_by_name['ControllerMeta'] = _CONTROLLERMETA
DESCRIPTOR.extensions_by_name['notify'] = notify
>>>>>>> a665b01... backup
=======
)

DESCRIPTOR.message_types_by_name['ControllerProto'] = _CONTROLLERPROTO
>>>>>>> bd6016103fecc4c9bae8b8be8002b50d7f3e8f55
=======
  serialized_start=33,
  serialized_end=234,
)

<<<<<<< HEAD
DESCRIPTOR.message_types_by_name['ControllerMeta'] = _CONTROLLERMETA
>>>>>>> ffae88c... backup
=======
DESCRIPTOR.message_types_by_name['ControllerProto'] = _CONTROLLERPROTO
>>>>>>> 4657d14... update

ControllerProto = _reflection.GeneratedProtocolMessageType('ControllerProto', (_message.Message,), dict(
  DESCRIPTOR = _CONTROLLERPROTO,
  __module__ = 'controller_pb2'
  # @@protoc_insertion_point(class_scope:maid.proto.ControllerProto)
  ))
_sym_db.RegisterMessage(ControllerProto)


# @@protoc_insertion_point(module_scope)
