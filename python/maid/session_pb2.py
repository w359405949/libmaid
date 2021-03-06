# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: maid/session.proto

import sys
_b=sys.version_info[0]<3 and (lambda x:x) or (lambda x:x.encode('latin1'))
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
from google.protobuf import descriptor_pb2
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


import maid.controller_pb2


DESCRIPTOR = _descriptor.FileDescriptor(
  name='maid/session.proto',
  package='maid.proto',
  serialized_pb=_b('\n\x12maid/session.proto\x12\nmaid.proto\x1a\x15maid/controller.proto\";\n\x0cSessionProto\x12\n\n\x02id\x18\x01 \x01(\t\x12\x14\n\x0c\x65xpired_time\x18\x02 \x01(\x04*\t\x08\xe8\x07\x10\x80\x80\x80\x80\x02:G\n\x07session\x12\x1b.maid.proto.ControllerProto\x18\xea\x07 \x01(\x0b\x32\x18.maid.proto.SessionProto')
  ,
  dependencies=[maid.controller_pb2.DESCRIPTOR,])
_sym_db.RegisterFileDescriptor(DESCRIPTOR)


SESSION_FIELD_NUMBER = 1002
session = _descriptor.FieldDescriptor(
  name='session', full_name='maid.proto.session', index=0,
  number=1002, type=11, cpp_type=10, label=1,
  has_default_value=False, default_value=None,
  message_type=None, enum_type=None, containing_type=None,
  is_extension=True, extension_scope=None,
  options=None)


_SESSIONPROTO = _descriptor.Descriptor(
  name='SessionProto',
  full_name='maid.proto.SessionProto',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='id', full_name='maid.proto.SessionProto.id', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='expired_time', full_name='maid.proto.SessionProto.expired_time', index=1,
      number=2, type=4, cpp_type=4, label=1,
      has_default_value=False, default_value=0,
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
  serialized_start=57,
  serialized_end=116,
)

DESCRIPTOR.message_types_by_name['SessionProto'] = _SESSIONPROTO
DESCRIPTOR.extensions_by_name['session'] = session

SessionProto = _reflection.GeneratedProtocolMessageType('SessionProto', (_message.Message,), dict(
  DESCRIPTOR = _SESSIONPROTO,
  __module__ = 'maid.session_pb2'
  # @@protoc_insertion_point(class_scope:maid.proto.SessionProto)
  ))
_sym_db.RegisterMessage(SessionProto)

session.message_type = _SESSIONPROTO
maid.controller_pb2.ControllerProto.RegisterExtension(session)

# @@protoc_insertion_point(module_scope)
