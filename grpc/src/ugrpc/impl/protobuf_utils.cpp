#include <ugrpc/impl/protobuf_utils.hpp>

#include <userver/ugrpc/protobuf_visit.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

void TrimSecrets(google::protobuf::Message& message) {
  static ugrpc::FieldsVisitor kSecretVisitor(
      [](const google::protobuf::Descriptor&,
         const google::protobuf::FieldDescriptor& field) {
        return GetFieldOptions(field).secret();
      });

  kSecretVisitor.VisitRecursive(
      message, [](google::protobuf::Message& message,
                  const google::protobuf::FieldDescriptor& field) {
        const google::protobuf::Reflection* reflection =
            message.GetReflection();
        UINVARIANT(reflection, "reflection is nullptr");
        reflection->ClearField(&message, &field);
      });
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
