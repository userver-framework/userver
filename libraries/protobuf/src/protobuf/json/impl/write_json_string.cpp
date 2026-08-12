#include <protobuf/json/impl/write_json_string.hpp>

#include <limits>
#include <string>

#include <google/protobuf/message.h>

#include <protobuf/json/impl/field_error.hpp>
#include <protobuf/json/impl/proto_message_visitor.hpp>
#include <protobuf/json/impl/string_writer.hpp>
#include <userver/protobuf/json/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::impl {

std::string WriteMessageToJsonString(const ::google::protobuf::Message& message, const PrintOptions& options) {
    StringWriter string_writer{std::numeric_limits<std::size_t>::max()};

    ProtoMessageVisitor visitor{string_writer};
    visitor.SetAlwaysPrintFieldsWithNoPresence(options.always_print_fields_with_no_presence);
    visitor.SetAlwaysPrintEnumsAsInts(options.always_print_enums_as_ints);
    visitor.SetPreserveProtoFieldNames(options.preserve_proto_field_names);
    visitor.SetExpandAny(!options.nonportable_raw_any);

    try {
        visitor(message);
    } catch (const FieldError& error) {
        throw PrintError(error.GetCode<PrintErrorCode>(), error.GetPath(), error.GetDescription());
    }

    return string_writer.GetString();
}

}  // namespace protobuf::json::impl

USERVER_NAMESPACE_END
