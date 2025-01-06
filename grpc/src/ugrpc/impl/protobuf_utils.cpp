#include <ugrpc/impl/protobuf_utils.hpp>

#include <userver/compiler/thread_local.hpp>
#include <userver/ugrpc/protobuf_visit.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

namespace {

compiler::ThreadLocal kSecretFieldsVisitor = [] {
    return ugrpc::FieldsVisitor(
        [](const google::protobuf::FieldDescriptor& field) { return GetFieldOptions(field).secret(); },
        ugrpc::FieldsVisitor::LockBehavior::kNone
    );
};

}  // namespace

const userver::FieldOptions& GetFieldOptions(const google::protobuf::FieldDescriptor& field) {
    return field.options().GetExtension(userver::field);
}

bool IsMessage(const google::protobuf::FieldDescriptor& field) {
    return field.type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE ||
           field.type() == google::protobuf::FieldDescriptor::TYPE_GROUP;
}

bool HasSecrets(const google::protobuf::Message& message) {
    auto visitor = kSecretFieldsVisitor.Use();
    return visitor->ContainsSelected(message.GetDescriptor());
}

void TrimSecrets(google::protobuf::Message& message) {
    auto visitor = kSecretFieldsVisitor.Use();
    visitor->VisitRecursive(
        message,
        [](google::protobuf::Message& message, const google::protobuf::FieldDescriptor& field) {
            const google::protobuf::Reflection* reflection = message.GetReflection();
            UINVARIANT(reflection, "reflection is nullptr");
            reflection->ClearField(&message, &field);
        }
    );
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
