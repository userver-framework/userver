#include <protobuf/json/impl/write_debug_string.hpp>

#include <protobuf/json/impl/field_error.hpp>
#include <protobuf/json/impl/proto_message_visitor.hpp>
#include <protobuf/json/impl/string_writer.hpp>
#include <userver/protobuf/json/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::impl {

std::string WriteMessageToDebugString(const ::google::protobuf::Message& message, std::size_t limit) {
    // Debug/logging tweaks on top of plain ProtoJSON: expand `google.protobuf.Any` but fall back to its raw
    // representation instead of failing, and redact `[debug_redact = true]` fields.
    StringWriter string_writer{limit};

    ProtoMessageVisitor visitor{string_writer};
    visitor.SetPreserveProtoFieldNames(true);
    visitor.SetExpandAny(true);
    visitor.SetExpandAnyFallbackToRaw(true);
    visitor.SetRedactDebugString(true);

    try {
        visitor(message);
    } catch (const FieldError& error) {
        throw PrintError(error.GetCode<PrintErrorCode>(), error.GetPath(), error.GetDescription());
    }

    return string_writer.GetString();
}

}  // namespace protobuf::json::impl

USERVER_NAMESPACE_END
