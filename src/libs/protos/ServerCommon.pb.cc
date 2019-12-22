// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: ServerCommon.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "ServerCommon.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/port.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace protobuf_ServerCommon_2eproto {


namespace {

const ::google::protobuf::EnumDescriptor* file_level_enum_descriptors[1];

}  // namespace

const ::google::protobuf::uint32 TableStruct::offsets[] = { ~0u };
static const ::google::protobuf::internal::MigrationSchema* schemas = NULL;
static const ::google::protobuf::Message* const* file_default_instances = NULL;
namespace {

void protobuf_AssignDescriptors() {
  AddDescriptors();
  ::google::protobuf::MessageFactory* factory = NULL;
  AssignDescriptors(
      "ServerCommon.proto", schemas, file_default_instances, TableStruct::offsets, factory,
      NULL, file_level_enum_descriptors, NULL);
}

void protobuf_AssignDescriptorsOnce() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::google::protobuf::GoogleOnceInit(&once, &protobuf_AssignDescriptors);
}

void protobuf_RegisterTypes(const ::std::string&) GOOGLE_ATTRIBUTE_COLD;
void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
}

}  // namespace

void TableStruct::Shutdown() {
}

void TableStruct::InitDefaultsImpl() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::internal::InitProtobufDefaults();
}

void InitDefaults() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::google::protobuf::GoogleOnceInit(&once, &TableStruct::InitDefaultsImpl);
}
void AddDescriptorsImpl() {
  InitDefaults();
  static const char descriptor[] = {
      "\n\022ServerCommon.proto*\301\002\n\013ServerError\022\006\n\002"
      "OK\020\000\022\021\n\rACCOUNT_EXIST\020\001\022\035\n\031ACCOUNT_OR_PA"
      "SSWORD_ERROR\020\002\022\r\n\tNOT_FOUND\020\003\022\024\n\020SERVER_"
      "NOT_READY\020\004\022\026\n\022FREQUENT_OPERATION\020\005\022\021\n\rI"
      "NVALID_TOKEN\020\006\022\024\n\020OBJECT_NOT_FOUND\020\007\022\013\n\007"
      "PLAYING\020\010\022\020\n\014MATCH_FAILED\020\t\022\036\n\032CREATE_RO"
      "OM_PROCESS_FAILED\020\n\022\013\n\007TIMEOUT\020\013\022\021\n\rCONN"
      "ECT_ERROR\020\014\022\020\n\014DISCONNECTED\020\r\022\020\n\014CREATE_"
      "ERROR\020\016\022\017\n\013AUTH_FAILED\020\017b\006proto3"
  };
  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
      descriptor, 352);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "ServerCommon.proto", &protobuf_RegisterTypes);
  ::google::protobuf::internal::OnShutdown(&TableStruct::Shutdown);
}

void AddDescriptors() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::google::protobuf::GoogleOnceInit(&once, &AddDescriptorsImpl);
}
// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer {
  StaticDescriptorInitializer() {
    AddDescriptors();
  }
} static_descriptor_initializer;

}  // namespace protobuf_ServerCommon_2eproto

const ::google::protobuf::EnumDescriptor* ServerError_descriptor() {
  protobuf_ServerCommon_2eproto::protobuf_AssignDescriptorsOnce();
  return protobuf_ServerCommon_2eproto::file_level_enum_descriptors[0];
}
bool ServerError_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
      return true;
    default:
      return false;
  }
}


// @@protoc_insertion_point(namespace_scope)

// @@protoc_insertion_point(global_scope)
