#include <ugrpc/impl/protobuf_utils.hpp>

#include <userver/ugrpc/protobuf_visit.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

bool IsFieldSecret(const google::protobuf::FieldDescriptor* field) {
  return GetFieldOptions(field).secret();
}

void TrimSecrets(google::protobuf::Message& message) {
  ugrpc::VisitFieldsRecursive(
      message, [](google::protobuf::Message& message,
                  const google::protobuf::FieldDescriptor& field) {
        if (IsFieldSecret(&field)) {
          const google::protobuf::Reflection* reflection =
              message.GetReflection();
          UINVARIANT(reflection, "reflection is nullptr");
          reflection->ClearField(&message, &field);
        }
      });
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
