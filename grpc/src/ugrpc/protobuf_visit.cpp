#include <userver/ugrpc/protobuf_visit.hpp>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/reflection.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace {

constexpr int kMaxRecursionLimit = 100;

void VisitMessagesRecursiveImpl(google::protobuf::Message& message,
                                MessageVisitor callback,
                                const int recursion_limit) {
  UINVARIANT(recursion_limit > 0,
             "Recursion limit reached while traversing protobuf Message.");

  // Apply to this message
  callback(message);

  // Loop over nested structs
  VisitFields(
      message,
      [&callback, &recursion_limit](
          google::protobuf::Message& message,
          const google::protobuf::FieldDescriptor& field) -> void {
        // Not a nested message
        if (field.type() != google::protobuf::FieldDescriptor::TYPE_MESSAGE &&
            field.type() != google::protobuf::FieldDescriptor::TYPE_GROUP) {
          return;
        }

        // Get reflection
        const google::protobuf::Reflection* reflection =
            message.GetReflection();
        UINVARIANT(reflection, "reflection is nullptr");

        if (field.is_repeated()) {
          // Repeated types (including maps)
          const int repeated_size = reflection->FieldSize(message, &field);
          for (int i = 0; i < repeated_size; ++i) {
            google::protobuf::Message* msg =
                reflection->MutableRepeatedMessage(&message, &field, i);
            UINVARIANT(msg, "msg is nullptr");
            VisitMessagesRecursiveImpl(*msg, callback, recursion_limit - 1);
          }
        } else if (reflection->HasField(message, &field)) {
          // Primitive types
          google::protobuf::Message* msg =
              reflection->MutableMessage(&message, &field);
          UINVARIANT(msg, "msg is nullptr");
          VisitMessagesRecursiveImpl(*msg, callback, recursion_limit - 1);
        }
      });
}

}  // namespace

void VisitFields(google::protobuf::Message& message, FieldVisitor callback) {
  // Get descriptor
  const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
  UINVARIANT(descriptor, "descriptor is nullptr");

  // Get reflection
  const google::protobuf::Reflection* reflection = message.GetReflection();
  UINVARIANT(reflection, "reflection is nullptr");

  for (int field_index = 0; field_index < descriptor->field_count();
       ++field_index) {
    // Get field descriptor
    const google::protobuf::FieldDescriptor* field =
        descriptor->field(field_index);
    UINVARIANT(field, "field is nullptr");

    if (field->is_repeated()) {
      // Repeated types (including maps)
      if (reflection->FieldSize(message, field) > 0) {
        callback(message, *field);
      }
    } else if (reflection->HasField(message, field)) {
      // Primitive types
      callback(message, *field);
    }
  }
}

void VisitMessagesRecursive(google::protobuf::Message& message,
                            MessageVisitor callback) {
  VisitMessagesRecursiveImpl(message, callback, kMaxRecursionLimit);
}

void VisitFieldsRecursive(google::protobuf::Message& message,
                          FieldVisitor callback) {
  VisitMessagesRecursiveImpl(
      message,
      [&](google::protobuf::Message& message) -> void {
        VisitFields(message, callback);
      },
      kMaxRecursionLimit);
}

}  // namespace ugrpc

USERVER_NAMESPACE_END
