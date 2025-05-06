#include <ugrpc/impl/protobuf_utils.hpp>

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>

#include <boost/container/small_vector.hpp>

#include <userver/compiler/thread_local.hpp>
#include <userver/utils/numeric_cast.hpp>

#include <userver/ugrpc/protobuf_visit.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

namespace {

[[maybe_unused]] bool HasDebugRedactOption([[maybe_unused]] const google::protobuf::FieldDescriptor& field) {
#if GOOGLE_PROTOBUF_VERSION >= 3022000
    return field.options().debug_redact();
#else
    return false;
#endif
}

#ifdef ARCADIA_ROOT
compiler::ThreadLocal kSecretFieldsVisitor = [] {
    return ugrpc::FieldsVisitor(
        [](const google::protobuf::FieldDescriptor& field) {
            // TODO enable this check.
            if constexpr (false) {
                UASSERT_MSG(
                    !GetFieldOptions(field).secret(), "Please use debug_redact instead of (userver.field).secret"
                );
            }
            return field.options().debug_redact() || GetFieldOptions(field).secret();
        },
        ugrpc::FieldsVisitor::LockBehavior::kNone
    );
};
#else
/// [fields visitor]
compiler::ThreadLocal kSecretFieldsVisitor = [] {
    return ugrpc::FieldsVisitor(
        [](const google::protobuf::FieldDescriptor& field) {
            return HasDebugRedactOption(field) || GetFieldOptions(field).secret();
        },
        ugrpc::FieldsVisitor::LockBehavior::kNone
    );
};
/// [fields visitor]
#endif

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

std::string ToLimitedString(const google::protobuf::Message& message, std::size_t limit) {
    boost::container::small_vector<char, 1024> output_buffer{limit, boost::container::default_init};
    google::protobuf::io::ArrayOutputStream output_stream{output_buffer.data(), utils::numeric_cast<int>(limit)};

    google::protobuf::TextFormat::Printer printer;
    printer.SetUseUtf8StringEscaping(true);
    printer.SetExpandAny(true);

    printer.Print(message, &output_stream);

    return std::string{output_buffer.data(), static_cast<std::size_t>(output_stream.ByteCount())};
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
