// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: controller.proto

#ifndef PROTOBUF_controller_2eproto__INCLUDED
#define PROTOBUF_controller_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2006000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2006000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)

namespace maid {
namespace proto {

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_controller_2eproto();
void protobuf_AssignDesc_controller_2eproto();
void protobuf_ShutdownFile_controller_2eproto();

class ControllerMeta;

// ===================================================================

class ControllerMeta : public ::google::protobuf::Message {
 public:
  ControllerMeta();
  virtual ~ControllerMeta();

  ControllerMeta(const ControllerMeta& from);

  inline ControllerMeta& operator=(const ControllerMeta& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const ControllerMeta& default_instance();

  void Swap(ControllerMeta* other);

  // implements Message ----------------------------------------------

  ControllerMeta* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const ControllerMeta& from);
  void MergeFrom(const ControllerMeta& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional string method_name = 2;
  inline bool has_method_name() const;
  inline void clear_method_name();
  static const int kMethodNameFieldNumber = 2;
  inline const ::std::string& method_name() const;
  inline void set_method_name(const ::std::string& value);
  inline void set_method_name(const char* value);
  inline void set_method_name(const char* value, size_t size);
  inline ::std::string* mutable_method_name();
  inline ::std::string* release_method_name();
  inline void set_allocated_method_name(::std::string* method_name);

  // optional uint32 transmit_id = 3;
  inline bool has_transmit_id() const;
  inline void clear_transmit_id();
  static const int kTransmitIdFieldNumber = 3;
  inline ::google::protobuf::uint32 transmit_id() const;
  inline void set_transmit_id(::google::protobuf::uint32 value);

  // optional bool stub = 4;
  inline bool has_stub() const;
  inline void clear_stub();
  static const int kStubFieldNumber = 4;
  inline bool stub() const;
  inline void set_stub(bool value);

  // optional bool is_canceled = 5;
  inline bool has_is_canceled() const;
  inline void clear_is_canceled();
  static const int kIsCanceledFieldNumber = 5;
  inline bool is_canceled() const;
  inline void set_is_canceled(bool value);

  // optional bool failed = 6;
  inline bool has_failed() const;
  inline void clear_failed();
  static const int kFailedFieldNumber = 6;
  inline bool failed() const;
  inline void set_failed(bool value);

  // optional string error_text = 7;
  inline bool has_error_text() const;
  inline void clear_error_text();
  static const int kErrorTextFieldNumber = 7;
  inline const ::std::string& error_text() const;
  inline void set_error_text(const ::std::string& value);
  inline void set_error_text(const char* value);
  inline void set_error_text(const char* value, size_t size);
  inline ::std::string* mutable_error_text();
  inline ::std::string* release_error_text();
  inline void set_allocated_error_text(::std::string* error_text);

  // optional bool notify = 9;
  inline bool has_notify() const;
  inline void clear_notify();
  static const int kNotifyFieldNumber = 9;
  inline bool notify() const;
  inline void set_notify(bool value);

  // optional bytes message = 10;
  inline bool has_message() const;
  inline void clear_message();
  static const int kMessageFieldNumber = 10;
  inline const ::std::string& message() const;
  inline void set_message(const ::std::string& value);
  inline void set_message(const char* value);
  inline void set_message(const void* value, size_t size);
  inline ::std::string* mutable_message();
  inline ::std::string* release_message();
  inline void set_allocated_message(::std::string* message);

  // optional string full_service_name = 11;
  inline bool has_full_service_name() const;
  inline void clear_full_service_name();
  static const int kFullServiceNameFieldNumber = 11;
  inline const ::std::string& full_service_name() const;
  inline void set_full_service_name(const ::std::string& value);
  inline void set_full_service_name(const char* value);
  inline void set_full_service_name(const char* value, size_t size);
  inline ::std::string* mutable_full_service_name();
  inline ::std::string* release_full_service_name();
  inline void set_allocated_full_service_name(::std::string* full_service_name);

  // @@protoc_insertion_point(class_scope:maid.proto.ControllerMeta)
 private:
  inline void set_has_method_name();
  inline void clear_has_method_name();
  inline void set_has_transmit_id();
  inline void clear_has_transmit_id();
  inline void set_has_stub();
  inline void clear_has_stub();
  inline void set_has_is_canceled();
  inline void clear_has_is_canceled();
  inline void set_has_failed();
  inline void clear_has_failed();
  inline void set_has_error_text();
  inline void clear_has_error_text();
  inline void set_has_notify();
  inline void clear_has_notify();
  inline void set_has_message();
  inline void clear_has_message();
  inline void set_has_full_service_name();
  inline void clear_has_full_service_name();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::std::string* method_name_;
  ::google::protobuf::uint32 transmit_id_;
  bool stub_;
  bool is_canceled_;
  bool failed_;
  bool notify_;
  ::std::string* error_text_;
  ::std::string* message_;
  ::std::string* full_service_name_;
  friend void  protobuf_AddDesc_controller_2eproto();
  friend void protobuf_AssignDesc_controller_2eproto();
  friend void protobuf_ShutdownFile_controller_2eproto();

  void InitAsDefaultInstance();
  static ControllerMeta* default_instance_;
};
// ===================================================================


// ===================================================================

// ControllerMeta

// optional string method_name = 2;
inline bool ControllerMeta::has_method_name() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void ControllerMeta::set_has_method_name() {
  _has_bits_[0] |= 0x00000001u;
}
inline void ControllerMeta::clear_has_method_name() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void ControllerMeta::clear_method_name() {
  if (method_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    method_name_->clear();
  }
  clear_has_method_name();
}
inline const ::std::string& ControllerMeta::method_name() const {
  // @@protoc_insertion_point(field_get:maid.proto.ControllerMeta.method_name)
  return *method_name_;
}
inline void ControllerMeta::set_method_name(const ::std::string& value) {
  set_has_method_name();
  if (method_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    method_name_ = new ::std::string;
  }
  method_name_->assign(value);
  // @@protoc_insertion_point(field_set:maid.proto.ControllerMeta.method_name)
}
inline void ControllerMeta::set_method_name(const char* value) {
  set_has_method_name();
  if (method_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    method_name_ = new ::std::string;
  }
  method_name_->assign(value);
  // @@protoc_insertion_point(field_set_char:maid.proto.ControllerMeta.method_name)
}
inline void ControllerMeta::set_method_name(const char* value, size_t size) {
  set_has_method_name();
  if (method_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    method_name_ = new ::std::string;
  }
  method_name_->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:maid.proto.ControllerMeta.method_name)
}
inline ::std::string* ControllerMeta::mutable_method_name() {
  set_has_method_name();
  if (method_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    method_name_ = new ::std::string;
  }
  // @@protoc_insertion_point(field_mutable:maid.proto.ControllerMeta.method_name)
  return method_name_;
}
inline ::std::string* ControllerMeta::release_method_name() {
  clear_has_method_name();
  if (method_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    return NULL;
  } else {
    ::std::string* temp = method_name_;
    method_name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
    return temp;
  }
}
inline void ControllerMeta::set_allocated_method_name(::std::string* method_name) {
  if (method_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete method_name_;
  }
  if (method_name) {
    set_has_method_name();
    method_name_ = method_name;
  } else {
    clear_has_method_name();
    method_name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  }
  // @@protoc_insertion_point(field_set_allocated:maid.proto.ControllerMeta.method_name)
}

// optional uint32 transmit_id = 3;
inline bool ControllerMeta::has_transmit_id() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void ControllerMeta::set_has_transmit_id() {
  _has_bits_[0] |= 0x00000002u;
}
inline void ControllerMeta::clear_has_transmit_id() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void ControllerMeta::clear_transmit_id() {
  transmit_id_ = 0u;
  clear_has_transmit_id();
}
inline ::google::protobuf::uint32 ControllerMeta::transmit_id() const {
  // @@protoc_insertion_point(field_get:maid.proto.ControllerMeta.transmit_id)
  return transmit_id_;
}
inline void ControllerMeta::set_transmit_id(::google::protobuf::uint32 value) {
  set_has_transmit_id();
  transmit_id_ = value;
  // @@protoc_insertion_point(field_set:maid.proto.ControllerMeta.transmit_id)
}

// optional bool stub = 4;
inline bool ControllerMeta::has_stub() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void ControllerMeta::set_has_stub() {
  _has_bits_[0] |= 0x00000004u;
}
inline void ControllerMeta::clear_has_stub() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void ControllerMeta::clear_stub() {
  stub_ = false;
  clear_has_stub();
}
inline bool ControllerMeta::stub() const {
  // @@protoc_insertion_point(field_get:maid.proto.ControllerMeta.stub)
  return stub_;
}
inline void ControllerMeta::set_stub(bool value) {
  set_has_stub();
  stub_ = value;
  // @@protoc_insertion_point(field_set:maid.proto.ControllerMeta.stub)
}

// optional bool is_canceled = 5;
inline bool ControllerMeta::has_is_canceled() const {
  return (_has_bits_[0] & 0x00000008u) != 0;
}
inline void ControllerMeta::set_has_is_canceled() {
  _has_bits_[0] |= 0x00000008u;
}
inline void ControllerMeta::clear_has_is_canceled() {
  _has_bits_[0] &= ~0x00000008u;
}
inline void ControllerMeta::clear_is_canceled() {
  is_canceled_ = false;
  clear_has_is_canceled();
}
inline bool ControllerMeta::is_canceled() const {
  // @@protoc_insertion_point(field_get:maid.proto.ControllerMeta.is_canceled)
  return is_canceled_;
}
inline void ControllerMeta::set_is_canceled(bool value) {
  set_has_is_canceled();
  is_canceled_ = value;
  // @@protoc_insertion_point(field_set:maid.proto.ControllerMeta.is_canceled)
}

// optional bool failed = 6;
inline bool ControllerMeta::has_failed() const {
  return (_has_bits_[0] & 0x00000010u) != 0;
}
inline void ControllerMeta::set_has_failed() {
  _has_bits_[0] |= 0x00000010u;
}
inline void ControllerMeta::clear_has_failed() {
  _has_bits_[0] &= ~0x00000010u;
}
inline void ControllerMeta::clear_failed() {
  failed_ = false;
  clear_has_failed();
}
inline bool ControllerMeta::failed() const {
  // @@protoc_insertion_point(field_get:maid.proto.ControllerMeta.failed)
  return failed_;
}
inline void ControllerMeta::set_failed(bool value) {
  set_has_failed();
  failed_ = value;
  // @@protoc_insertion_point(field_set:maid.proto.ControllerMeta.failed)
}

// optional string error_text = 7;
inline bool ControllerMeta::has_error_text() const {
  return (_has_bits_[0] & 0x00000020u) != 0;
}
inline void ControllerMeta::set_has_error_text() {
  _has_bits_[0] |= 0x00000020u;
}
inline void ControllerMeta::clear_has_error_text() {
  _has_bits_[0] &= ~0x00000020u;
}
inline void ControllerMeta::clear_error_text() {
  if (error_text_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    error_text_->clear();
  }
  clear_has_error_text();
}
inline const ::std::string& ControllerMeta::error_text() const {
  // @@protoc_insertion_point(field_get:maid.proto.ControllerMeta.error_text)
  return *error_text_;
}
inline void ControllerMeta::set_error_text(const ::std::string& value) {
  set_has_error_text();
  if (error_text_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    error_text_ = new ::std::string;
  }
  error_text_->assign(value);
  // @@protoc_insertion_point(field_set:maid.proto.ControllerMeta.error_text)
}
inline void ControllerMeta::set_error_text(const char* value) {
  set_has_error_text();
  if (error_text_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    error_text_ = new ::std::string;
  }
  error_text_->assign(value);
  // @@protoc_insertion_point(field_set_char:maid.proto.ControllerMeta.error_text)
}
inline void ControllerMeta::set_error_text(const char* value, size_t size) {
  set_has_error_text();
  if (error_text_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    error_text_ = new ::std::string;
  }
  error_text_->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:maid.proto.ControllerMeta.error_text)
}
inline ::std::string* ControllerMeta::mutable_error_text() {
  set_has_error_text();
  if (error_text_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    error_text_ = new ::std::string;
  }
  // @@protoc_insertion_point(field_mutable:maid.proto.ControllerMeta.error_text)
  return error_text_;
}
inline ::std::string* ControllerMeta::release_error_text() {
  clear_has_error_text();
  if (error_text_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    return NULL;
  } else {
    ::std::string* temp = error_text_;
    error_text_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
    return temp;
  }
}
inline void ControllerMeta::set_allocated_error_text(::std::string* error_text) {
  if (error_text_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete error_text_;
  }
  if (error_text) {
    set_has_error_text();
    error_text_ = error_text;
  } else {
    clear_has_error_text();
    error_text_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  }
  // @@protoc_insertion_point(field_set_allocated:maid.proto.ControllerMeta.error_text)
}

// optional bool notify = 9;
inline bool ControllerMeta::has_notify() const {
  return (_has_bits_[0] & 0x00000040u) != 0;
}
inline void ControllerMeta::set_has_notify() {
  _has_bits_[0] |= 0x00000040u;
}
inline void ControllerMeta::clear_has_notify() {
  _has_bits_[0] &= ~0x00000040u;
}
inline void ControllerMeta::clear_notify() {
  notify_ = false;
  clear_has_notify();
}
inline bool ControllerMeta::notify() const {
  // @@protoc_insertion_point(field_get:maid.proto.ControllerMeta.notify)
  return notify_;
}
inline void ControllerMeta::set_notify(bool value) {
  set_has_notify();
  notify_ = value;
  // @@protoc_insertion_point(field_set:maid.proto.ControllerMeta.notify)
}

// optional bytes message = 10;
inline bool ControllerMeta::has_message() const {
  return (_has_bits_[0] & 0x00000080u) != 0;
}
inline void ControllerMeta::set_has_message() {
  _has_bits_[0] |= 0x00000080u;
}
inline void ControllerMeta::clear_has_message() {
  _has_bits_[0] &= ~0x00000080u;
}
inline void ControllerMeta::clear_message() {
  if (message_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    message_->clear();
  }
  clear_has_message();
}
inline const ::std::string& ControllerMeta::message() const {
  // @@protoc_insertion_point(field_get:maid.proto.ControllerMeta.message)
  return *message_;
}
inline void ControllerMeta::set_message(const ::std::string& value) {
  set_has_message();
  if (message_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    message_ = new ::std::string;
  }
  message_->assign(value);
  // @@protoc_insertion_point(field_set:maid.proto.ControllerMeta.message)
}
inline void ControllerMeta::set_message(const char* value) {
  set_has_message();
  if (message_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    message_ = new ::std::string;
  }
  message_->assign(value);
  // @@protoc_insertion_point(field_set_char:maid.proto.ControllerMeta.message)
}
inline void ControllerMeta::set_message(const void* value, size_t size) {
  set_has_message();
  if (message_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    message_ = new ::std::string;
  }
  message_->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:maid.proto.ControllerMeta.message)
}
inline ::std::string* ControllerMeta::mutable_message() {
  set_has_message();
  if (message_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    message_ = new ::std::string;
  }
  // @@protoc_insertion_point(field_mutable:maid.proto.ControllerMeta.message)
  return message_;
}
inline ::std::string* ControllerMeta::release_message() {
  clear_has_message();
  if (message_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    return NULL;
  } else {
    ::std::string* temp = message_;
    message_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
    return temp;
  }
}
inline void ControllerMeta::set_allocated_message(::std::string* message) {
  if (message_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete message_;
  }
  if (message) {
    set_has_message();
    message_ = message;
  } else {
    clear_has_message();
    message_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  }
  // @@protoc_insertion_point(field_set_allocated:maid.proto.ControllerMeta.message)
}

// optional string full_service_name = 11;
inline bool ControllerMeta::has_full_service_name() const {
  return (_has_bits_[0] & 0x00000100u) != 0;
}
inline void ControllerMeta::set_has_full_service_name() {
  _has_bits_[0] |= 0x00000100u;
}
inline void ControllerMeta::clear_has_full_service_name() {
  _has_bits_[0] &= ~0x00000100u;
}
inline void ControllerMeta::clear_full_service_name() {
  if (full_service_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    full_service_name_->clear();
  }
  clear_has_full_service_name();
}
inline const ::std::string& ControllerMeta::full_service_name() const {
  // @@protoc_insertion_point(field_get:maid.proto.ControllerMeta.full_service_name)
  return *full_service_name_;
}
inline void ControllerMeta::set_full_service_name(const ::std::string& value) {
  set_has_full_service_name();
  if (full_service_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    full_service_name_ = new ::std::string;
  }
  full_service_name_->assign(value);
  // @@protoc_insertion_point(field_set:maid.proto.ControllerMeta.full_service_name)
}
inline void ControllerMeta::set_full_service_name(const char* value) {
  set_has_full_service_name();
  if (full_service_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    full_service_name_ = new ::std::string;
  }
  full_service_name_->assign(value);
  // @@protoc_insertion_point(field_set_char:maid.proto.ControllerMeta.full_service_name)
}
inline void ControllerMeta::set_full_service_name(const char* value, size_t size) {
  set_has_full_service_name();
  if (full_service_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    full_service_name_ = new ::std::string;
  }
  full_service_name_->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:maid.proto.ControllerMeta.full_service_name)
}
inline ::std::string* ControllerMeta::mutable_full_service_name() {
  set_has_full_service_name();
  if (full_service_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    full_service_name_ = new ::std::string;
  }
  // @@protoc_insertion_point(field_mutable:maid.proto.ControllerMeta.full_service_name)
  return full_service_name_;
}
inline ::std::string* ControllerMeta::release_full_service_name() {
  clear_has_full_service_name();
  if (full_service_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    return NULL;
  } else {
    ::std::string* temp = full_service_name_;
    full_service_name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
    return temp;
  }
}
inline void ControllerMeta::set_allocated_full_service_name(::std::string* full_service_name) {
  if (full_service_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete full_service_name_;
  }
  if (full_service_name) {
    set_has_full_service_name();
    full_service_name_ = full_service_name;
  } else {
    clear_has_full_service_name();
    full_service_name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  }
  // @@protoc_insertion_point(field_set_allocated:maid.proto.ControllerMeta.full_service_name)
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace proto
}  // namespace maid

#ifndef SWIG
namespace google {
namespace protobuf {


}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_controller_2eproto__INCLUDED