#include <ugrpc/impl/protobuf_utils.hpp>

#include <memory>

#include <fmt/format.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/util/json_util.h>
#include <grpcpp/support/config.h>

#include <userver/compiler/thread_local.hpp>
#include <userver/logging/log.hpp>
#include <userver/ugrpc/protobuf_visit.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/log.hpp>

#include <userver/field_options.pb.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

namespace {

compiler::ThreadLocal kSecretVisitor = [] {
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

void TrimSecrets(google::protobuf::Message& message) {
    auto visitor = kSecretVisitor.Use();
    visitor->VisitRecursive(
        message,
        [](google::protobuf::Message& message, const google::protobuf::FieldDescriptor& field) {
            const google::protobuf::Reflection* reflection = message.GetReflection();
            UINVARIANT(reflection, "reflection is nullptr");
            reflection->ClearField(&message, &field);
        }
    );
}

std::string ToString(const google::protobuf::Message& message, const std::size_t max_msg_size) {
    return utils::log::ToLimitedUtf8(message.Utf8DebugString(), max_msg_size);
}

std::string ToJsonString(const google::protobuf::Message& message, const std::size_t max_msg_size) {
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = false;
    grpc::string as_json;

    const auto status = google::protobuf::util::MessageToJsonString(message, &as_json, options);
    if (!status.ok()) {
        std::string log = fmt::format("Error getting a json string: {}", status.ToString());
        LOG_WARNING() << log;
        return log;
    }
    return utils::log::ToLimitedUtf8(as_json, max_msg_size);
}

std::string ToLogString(const google::protobuf::Message& message, const std::size_t max_msg_size) {
    auto visitor = kSecretVisitor.Use();
    if (!visitor->ContainsSelected(message.GetDescriptor())) {
        return ToString(message, max_msg_size);
    }
    std::unique_ptr<google::protobuf::Message> trimmed{message.New()};
    trimmed->CopyFrom(message);
    TrimSecrets(*trimmed);
    return ToString(*trimmed, max_msg_size);
}

std::string ToJsonLogString(const google::protobuf::Message& message, const std::size_t max_msg_size) {
    auto visitor = kSecretVisitor.Use();
    if (!visitor->ContainsSelected(message.GetDescriptor())) {
        return ToJsonString(message, max_msg_size);
    }
    std::unique_ptr<google::protobuf::Message> trimmed{message.New()};
    trimmed->CopyFrom(message);
    TrimSecrets(*trimmed);
    return ToJsonString(*trimmed, max_msg_size);
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
