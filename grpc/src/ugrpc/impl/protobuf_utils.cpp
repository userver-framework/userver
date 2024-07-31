#include <ugrpc/impl/protobuf_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

namespace {

bool IsFieldSecret(const google::protobuf::FieldDescriptor* field) {
  return GetFieldOptions(field).secret();
}

void TrimSecretsRecursive(google::protobuf::Message& message,
                          int recursion_limit) {
  const auto* reflection = message.GetReflection();
  const auto* descriptor = message.GetDescriptor();
  for (int field_index = 0; field_index < descriptor->field_count();
       ++field_index) {
    const auto* field = descriptor->field(field_index);
    if (field->is_repeated()) {
      const auto repeated_size = reflection->FieldSize(message, field);
      if (0 < repeated_size) {
        if (IsFieldSecret(field) || recursion_limit < 0) {
          reflection->ClearField(&message, field);
        } else {
          if (google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE ==
              field->cpp_type()) {
            for (int i = 0; i < repeated_size; ++i) {
              TrimSecretsRecursive(
                  *reflection->MutableRepeatedMessage(&message, field, i),
                  recursion_limit - 1);
            }
          }
        }
      }
    } else if (reflection->HasField(message, field)) {
      if (IsFieldSecret(field) || recursion_limit < 0) {
        reflection->ClearField(&message, field);
      } else if (google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE ==
                 field->cpp_type()) {
        TrimSecretsRecursive(*reflection->MutableMessage(&message, field),
                             recursion_limit - 1);
      }
    }
  }
}

}  // namespace

void TrimSecrets(google::protobuf::Message& message) {
  constexpr int kMaxRecursionLimit = 100;
  TrimSecretsRecursive(message, kMaxRecursionLimit);
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
