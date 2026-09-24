#include <userver/ugrpc/proto_json.hpp>

#include <string>
#include <string_view>

#include <fmt/format.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/protobuf/exceptions.hpp>
#include <userver/protobuf/json/convert.hpp>
#include <userver/protobuf/json/convert_options.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace impl {

const google::protobuf::util::JsonPrintOptions kDefaultJsonPrintOptions = [] {
    google::protobuf::util::JsonPrintOptions options;
#if GOOGLE_PROTOBUF_VERSION >= 5026000
    options.always_print_fields_with_no_presence = true;
#else
    options.always_print_primitive_fields = true;
#endif
    return options;
}();

const google::protobuf::util::JsonParseOptions kDefaultJsonParseOptions = [] {
    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = false;
    options.case_insensitive_enum_parsing = false;
    return options;
}();

}  // namespace impl

namespace {

// 'add_whitespace' is handled separately: it does not affect formats::json::Value, and for the string output the
// result is pretty-printed with formats::json::ToPrettyString.
protobuf::json::PrintOptions ToPrintOptions(const google::protobuf::util::JsonPrintOptions& options) {
    protobuf::json::PrintOptions result;
#if GOOGLE_PROTOBUF_VERSION >= 5026000
    result.always_print_fields_with_no_presence = options.always_print_fields_with_no_presence;
#else
    result.always_print_fields_with_no_presence = options.always_print_primitive_fields;
#endif
    result.always_print_enums_as_ints = options.always_print_enums_as_ints;
    result.preserve_proto_field_names = options.preserve_proto_field_names;
    return result;
}

protobuf::json::ParseOptions ToParseOptions(const google::protobuf::util::JsonParseOptions& options) {
    protobuf::json::ParseOptions result;
    result.ignore_unknown_fields = options.ignore_unknown_fields;
    return result;
}

}  // namespace

namespace impl {

void FromJsonStringImpl(
    std::string_view json_string,
    google::protobuf::Message& output,
    const google::protobuf::util::JsonParseOptions& options
) {
    JsonToMessageImpl(formats::json::FromString(json_string), output, options);
}

void JsonToMessageImpl(
    const formats::json::Value& json,
    google::protobuf::Message& output,
    const google::protobuf::util::JsonParseOptions& options
) {
    const auto parse_options = ToParseOptions(options);
    protobuf::json::JsonToMessage(json, output, parse_options);
}

}  // namespace impl

formats::json::Value MessageToJson(const google::protobuf::Message& message) {
    return MessageToJson(message, impl::kDefaultJsonPrintOptions);
}

formats::json::Value MessageToJson(
    const google::protobuf::Message& message,
    const google::protobuf::util::JsonPrintOptions& options
) {
    const auto print_options = ToPrintOptions(options);
    return protobuf::json::MessageToJson(message, print_options);
}

std::string ToJsonString(const google::protobuf::Message& message) {
    return ToJsonString(message, impl::kDefaultJsonPrintOptions);
}

std::string ToJsonString(
    const google::protobuf::Message& message,
    const google::protobuf::util::JsonPrintOptions& options
) {
    if (options.add_whitespace) {
        return formats::json::ToPrettyString(MessageToJson(message, options));
    }

    const auto print_options = ToPrintOptions(options);
    return protobuf::json::MessageToJsonString(message, print_options);
}

}  // namespace ugrpc

namespace formats::serialize {

json::Value Serialize(const google::protobuf::Message& message, To<json::Value>) {
    return ugrpc::MessageToJson(message);
}

}  // namespace formats::serialize

namespace formats::parse {

google::protobuf::Value Parse(const json::Value& value, To<google::protobuf::Value>) {
    return ugrpc::JsonToMessage<google::protobuf::Value>(value);
}

google::protobuf::Struct Parse(const json::Value& value, To<google::protobuf::Struct>) {
    return ugrpc::JsonToMessage<google::protobuf::Struct>(value);
}

google::protobuf::ListValue Parse(const json::Value& value, To<google::protobuf::ListValue>) {
    return ugrpc::JsonToMessage<google::protobuf::ListValue>(value);
}

}  // namespace formats::parse

USERVER_NAMESPACE_END
