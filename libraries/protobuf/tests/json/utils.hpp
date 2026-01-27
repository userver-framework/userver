#pragma once

#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include <fmt/format.h>
#include <google/protobuf/message.h>
#include <gtest/gtest.h>

#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common.hpp>
#include <userver/protobuf/json/convert_options.hpp>
#include <userver/protobuf/json/exceptions.hpp>

#include "proto_json/messages.pb.h"

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPECT_PRINT_ERROR(EXPR, CODE, PATH)                             \
    try {                                                                \
        (EXPR);                                                          \
        ADD_FAILURE() << "Should throw 'PrintError' exception";          \
    } catch (const protobuf::json::PrintError& error) {                  \
        EXPECT_EQ(error.GetErrorInfo().GetCode(), (CODE));               \
        EXPECT_EQ(error.GetErrorInfo().GetPath(), (PATH));               \
    } catch (...) {                                                      \
        ADD_FAILURE() << "Unexpected exception, should be 'PrintError'"; \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPECT_PARSE_ERROR(EXPR, CODE, PATH)                             \
    try {                                                                \
        (EXPR);                                                          \
        ADD_FAILURE() << "Should throw 'ParseError' exception";          \
    } catch (const protobuf::json::ParseError& error) {                  \
        EXPECT_EQ(error.GetErrorInfo().GetCode(), (CODE));               \
        EXPECT_EQ(error.GetErrorInfo().GetPath(), (PATH));               \
    } catch (...) {                                                      \
        ADD_FAILURE() << "Unexpected exception, should be 'ParseError'"; \
    }

template <>
struct fmt::formatter<USERVER_NAMESPACE::protobuf::json::PrintOptions> {
    auto parse(fmt::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw fmt::format_error("invalid format");
        }
        return it;
    }

    template <typename FormatContext>
    auto format(const USERVER_NAMESPACE::protobuf::json::PrintOptions& options, FormatContext& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(
            ctx.out(),
            "{{always_print_fields_with_no_presence={}, always_print_enums_as_ints={}, preserve_proto_field_names={}}}",
            options.always_print_fields_with_no_presence,
            options.always_print_enums_as_ints,
            options.preserve_proto_field_names
        );
    }
};

template <>
struct fmt::formatter<USERVER_NAMESPACE::protobuf::json::ParseOptions> {
    auto parse(fmt::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw fmt::format_error("invalid format");
        }
        return it;
    }

    template <typename FormatContext>
    auto format(const USERVER_NAMESPACE::protobuf::json::ParseOptions& options, FormatContext& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "{{ignore_unknown_fields={}}}", options.ignore_unknown_fields);
    }
};

template <>
struct fmt::formatter<USERVER_NAMESPACE::protobuf::json::PrintErrorCode> {
    auto parse(fmt::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw fmt::format_error("invalid format");
        }
        return it;
    }

    template <typename FormatContext>
    auto format(const USERVER_NAMESPACE::protobuf::json::PrintErrorCode& code, FormatContext& ctx) const
        -> decltype(ctx.out()) {
        switch (code) {
            case USERVER_NAMESPACE::protobuf::json::PrintErrorCode::kInvalidValue:
                return fmt::format_to(ctx.out(), "kInvalidValue");
            default:
                return fmt::format_to(ctx.out(), "UNKNOWN");
        }
    }
};

template <>
struct fmt::formatter<USERVER_NAMESPACE::protobuf::json::ParseErrorCode> {
    auto parse(fmt::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw fmt::format_error("invalid format");
        }
        return it;
    }

    template <typename FormatContext>
    auto format(const USERVER_NAMESPACE::protobuf::json::ParseErrorCode& code, FormatContext& ctx) const
        -> decltype(ctx.out()) {
        switch (code) {
            case USERVER_NAMESPACE::protobuf::json::ParseErrorCode::kUnknownField:
                return fmt::format_to(ctx.out(), "kUnknownField");
            case USERVER_NAMESPACE::protobuf::json::ParseErrorCode::kUnknownEnum:
                return fmt::format_to(ctx.out(), "kUnknownEnum");
            case USERVER_NAMESPACE::protobuf::json::ParseErrorCode::kMultipleOneofFields:
                return fmt::format_to(ctx.out(), "kMultipleOneofFields");
            case USERVER_NAMESPACE::protobuf::json::ParseErrorCode::kInvalidType:
                return fmt::format_to(ctx.out(), "kInvalidType");
            case USERVER_NAMESPACE::protobuf::json::ParseErrorCode::kInvalidValue:
                return fmt::format_to(ctx.out(), "kInvalidValue");
            default:
                return fmt::format_to(ctx.out(), "UNKNOWN");
        }
    }
};

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

constexpr inline ::google::protobuf::NullValue kNullConst = ::google::protobuf::NullValue::NULL_VALUE;

class SampleError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct ProtoNullValue {};
constexpr inline ProtoNullValue kProtoNullValue{};

using ProtoValue = std::variant<
    std::monostate,
    ProtoNullValue,
    double,
    std::string,
    bool,
    std::vector<double>,
    std::map<std::string, std::string>>;

formats::json::Value PrepareJsonTestData(const std::string& json_data);

formats::json::Value CreateSampleJson(const ::google::protobuf::Message& message, const PrintOptions& options = {});

void InitSampleMessage(const std::string& json, ::google::protobuf::Message& message, const ParseOptions& options = {});

::google::protobuf::Value ProtoValueToNative(const ProtoValue& data);

struct Int32MessageData {
    std::int32_t field1 = 0;
    std::int32_t field2 = 0;
    std::int32_t field3 = 0;
};

struct UInt32MessageData {
    std::uint32_t field1 = 0;
    std::uint32_t field2 = 0;
};

struct Int64MessageData {
    std::int64_t field1 = 0;
    std::int64_t field2 = 0;
    std::int64_t field3 = 0;
};

struct UInt64MessageData {
    std::uint64_t field1 = 0;
    std::uint64_t field2 = 0;
};

struct FloatMessageData {
    float field1 = 0;
};

struct DoubleMessageData {
    double field1 = 0;
};

struct BoolMessageData {
    bool field1 = false;
};

struct StringMessageData {
    std::string field1 = {};
};

struct BytesMessageData {
    std::string field1 = {};
};

struct EnumMessageData {
    proto_json::messages::EnumMessage::Test field1 = proto_json::messages::EnumMessage::TEST_UNSPECIFIED;
};

struct NestedMessageData {
    std::optional<std::int32_t> field1 = {};
};

struct WrapperMessageData {
    std::optional<double> field1 = {};
    std::optional<float> field2 = {};
    std::optional<std::int64_t> field3 = {};
    std::optional<std::uint64_t> field4 = {};
    std::optional<std::int32_t> field5 = {};
    std::optional<std::uint32_t> field6 = {};
    std::optional<bool> field7 = {};
    std::optional<std::string> field8 = {};
    std::optional<std::string> field9 = {};
};

struct FieldMaskMessageData {
    std::optional<std::vector<std::string>> field1 = {};
};

struct DurationMessageData {
    std::int64_t seconds = 0;
    std::int32_t nanos = 0;
    bool do_not_set = false;
};

struct TimestampMessageData {
    std::int64_t seconds = 0;
    std::int32_t nanos = 0;
    bool do_not_set = false;
};

struct ValueMessageData {
    std::optional<std::variant<ProtoValue, std::vector<ProtoValue>, std::map<std::string, ProtoValue>>> field1 = {};
};

struct RawAnyData {
    std::string type_url = {};
    std::string value = {};
};

struct AnyMessageData {
    std::optional<std::variant<Int32MessageData, DurationMessageData, ValueMessageData, RawAnyData>> field1 = {};
    bool add_nesting = false;
};

struct OneofMessageData {
    std::optional<std::int32_t> field1 = {};
    std::optional<std::string> field2 = {};
};

struct RepeatedMessageData {
    std::vector<std::int32_t> field1 = {};
    std::vector<BoolMessageData> field2 = {};
    std::vector<DurationMessageData> field3 = {};
};

struct MapMessageData {
    std::map<std::int32_t, std::int32_t> field1 = {};
    std::map<std::uint32_t, double> field2 = {};
    std::map<std::int64_t, bool> field3 = {};
    std::map<std::uint64_t, std::string> field4 = {};
    std::map<bool, proto_json::messages::MapMessage::TestEnum> field5 = {};
    std::map<std::string, BoolMessageData> field6 = {};
    std::map<std::string, DurationMessageData> field7 = {};
};

struct NullValueMessageData {
    ::google::protobuf::NullValue field1 = {};
    std::optional<::google::protobuf::NullValue> field2 = {};
    std::vector<::google::protobuf::NullValue> field3 = {};
    std::map<std::int32_t, ::google::protobuf::NullValue> field4 = {};
};

struct PresenceMessageData {
    bool field1 = false;
    std::optional<bool> field2 = {};
    std::optional<Int32MessageData> field3 = {};
    std::vector<bool> field4 = {};
    std::map<std::int32_t, bool> field5 = {};
};

struct UnknownFieldMessageData {
    StringMessageData field1 = {};
    std::vector<StringMessageData> field2 = {};
    std::map<std::int32_t, StringMessageData> field3 = {};
};

proto_json::messages::Int32Message PrepareTestData(const Int32MessageData& message_data);
void CheckMessageEqual(const proto_json::messages::Int32Message& lhs, const proto_json::messages::Int32Message& rhs);

proto_json::messages::UInt32Message PrepareTestData(const UInt32MessageData& message_data);
void CheckMessageEqual(const proto_json::messages::UInt32Message& lhs, const proto_json::messages::UInt32Message& rhs);

proto_json::messages::Int64Message PrepareTestData(const Int64MessageData& message_data);
void CheckMessageEqual(const proto_json::messages::Int64Message& lhs, const proto_json::messages::Int64Message& rhs);

proto_json::messages::UInt64Message PrepareTestData(const UInt64MessageData& message_data);
void CheckMessageEqual(const proto_json::messages::UInt64Message& lhs, const proto_json::messages::UInt64Message& rhs);

proto_json::messages::FloatMessage PrepareTestData(const FloatMessageData& message_data);
void CheckMessageEqual(const proto_json::messages::FloatMessage& lhs, const proto_json::messages::FloatMessage& rhs);

proto_json::messages::DoubleMessage PrepareTestData(const DoubleMessageData& message_data);
void CheckMessageEqual(const proto_json::messages::DoubleMessage& lhs, const proto_json::messages::DoubleMessage& rhs);

proto_json::messages::BoolMessage PrepareTestData(const BoolMessageData& message_data);
void CheckMessageEqual(const proto_json::messages::BoolMessage& lhs, const proto_json::messages::BoolMessage& rhs);

proto_json::messages::StringMessage PrepareTestData(const StringMessageData& message_data);
void CheckMessageEqual(const proto_json::messages::StringMessage& lhs, const proto_json::messages::StringMessage& rhs);

proto_json::messages::BytesMessage PrepareTestData(const BytesMessageData& message_data);
void CheckMessageEqual(const proto_json::messages::BytesMessage& lhs, const proto_json::messages::BytesMessage& rhs);

proto_json::messages::EnumMessage PrepareTestData(const EnumMessageData& message_data);
void CheckMessageEqual(const proto_json::messages::EnumMessage& lhs, const proto_json::messages::EnumMessage& rhs);

proto_json::messages::NestedMessage PrepareTestData(const NestedMessageData& message_data);
void CheckMessageEqual(const proto_json::messages::NestedMessage& lhs, const proto_json::messages::NestedMessage& rhs);

proto_json::messages::WrapperMessage PrepareTestData(const WrapperMessageData& message_data);
void CheckMessageEqual(
    const proto_json::messages::WrapperMessage& lhs,
    const proto_json::messages::WrapperMessage& rhs
);

proto_json::messages::FieldMaskMessage PrepareTestData(const FieldMaskMessageData& message_data);
void CheckMessageEqual(const ::google::protobuf::FieldMask& lhs, const ::google::protobuf::FieldMask& rhs);
void CheckMessageEqual(
    const proto_json::messages::FieldMaskMessage& lhs,
    const proto_json::messages::FieldMaskMessage& rhs
);

proto_json::messages::DurationMessage PrepareTestData(const DurationMessageData& message_data);
void CheckMessageEqual(const ::google::protobuf::Duration& lhs, const ::google::protobuf::Duration& rhs);
void CheckMessageEqual(
    const proto_json::messages::DurationMessage& lhs,
    const proto_json::messages::DurationMessage& rhs
);

proto_json::messages::TimestampMessage PrepareTestData(const TimestampMessageData& message_data);
void CheckMessageEqual(const ::google::protobuf::Timestamp& lhs, const ::google::protobuf::Timestamp& rhs);
void CheckMessageEqual(
    const proto_json::messages::TimestampMessage& lhs,
    const proto_json::messages::TimestampMessage& rhs
);

proto_json::messages::ValueMessage PrepareTestData(const ValueMessageData& message_data);
void CheckMessageEqual(const ::google::protobuf::Value& lhs, const ::google::protobuf::Value& rhs);
void CheckMessageEqual(const ::google::protobuf::ListValue& lhs, const ::google::protobuf::ListValue& rhs);
void CheckMessageEqual(const ::google::protobuf::Struct& lhs, const ::google::protobuf::Struct& rhs);
void CheckMessageEqual(const proto_json::messages::ValueMessage& lhs, const proto_json::messages::ValueMessage& rhs);

proto_json::messages::AnyMessage PrepareTestData(const AnyMessageData& message_data);
void CheckMessageEqual(const ::google::protobuf::Any& lhs, const ::google::protobuf::Any& rhs);
void CheckMessageEqual(const proto_json::messages::AnyMessage& lhs, const proto_json::messages::AnyMessage& rhs);

proto_json::messages::OneofMessage PrepareTestData(const OneofMessageData& message_data);
void CheckMessageEqual(const proto_json::messages::OneofMessage& lhs, const proto_json::messages::OneofMessage& rhs);

proto_json::messages::RepeatedMessage PrepareTestData(const RepeatedMessageData& message_data);
void CheckMessageEqual(
    const proto_json::messages::RepeatedMessage& lhs,
    const proto_json::messages::RepeatedMessage& rhs
);

proto_json::messages::MapMessage PrepareTestData(const MapMessageData& message_data);
void CheckMessageEqual(const proto_json::messages::MapMessage& lhs, const proto_json::messages::MapMessage& rhs);

proto_json::messages::NullValueMessage PrepareTestData(const NullValueMessageData& message_data);
void CheckMessageEqual(
    const proto_json::messages::NullValueMessage& lhs,
    const proto_json::messages::NullValueMessage& rhs
);

proto_json::messages::PresenceMessage PrepareTestData(const PresenceMessageData& message_data);
void CheckMessageEqual(
    const proto_json::messages::PresenceMessage& lhs,
    const proto_json::messages::PresenceMessage& rhs
);

proto_json::messages::UnknownFieldMessage PrepareTestData(const UnknownFieldMessageData& message_data);
void CheckMessageEqual(
    const proto_json::messages::UnknownFieldMessage& lhs,
    const proto_json::messages::UnknownFieldMessage& rhs
);

struct FloatQuietNan {
    using Message = proto_json::messages::FloatMessage;
    static constexpr const char* kJson = R"({"field1":"NaN"})";
    static constexpr FloatMessageData kValue = {std::numeric_limits<float>::quiet_NaN()};
    static constexpr bool kSkip = !std::numeric_limits<float>::has_quiet_NaN;
};

struct FloatSignalingNan {
    using Message = proto_json::messages::FloatMessage;
    static constexpr const char* kJson = R"({"field1":"NaN"})";
    static constexpr FloatMessageData kValue = {std::numeric_limits<float>::signaling_NaN()};
    static constexpr bool kSkip = !std::numeric_limits<float>::has_signaling_NaN;
};

struct FloatPositiveInfinity {
    using Message = proto_json::messages::FloatMessage;
    static constexpr const char* kJson = R"({"field1":"Infinity"})";
    static constexpr FloatMessageData kValue = {std::numeric_limits<float>::infinity()};
    static constexpr bool kSkip = !std::numeric_limits<float>::has_infinity;
};

struct FloatNegativeInfinity {
    using Message = proto_json::messages::FloatMessage;
    static constexpr const char* kJson = R"({"field1":"-Infinity"})";
    static constexpr FloatMessageData kValue = {-std::numeric_limits<float>::infinity()};
    static constexpr bool kSkip = !std::numeric_limits<float>::has_infinity;
};

struct DoubleQuietNan {
    using Message = proto_json::messages::DoubleMessage;
    static constexpr const char* kJson = R"({"field1":"NaN"})";
    static constexpr DoubleMessageData kValue = {std::numeric_limits<double>::quiet_NaN()};
    static constexpr bool kSkip = !std::numeric_limits<double>::has_quiet_NaN;
};

struct DoubleSignalingNan {
    using Message = proto_json::messages::DoubleMessage;
    static constexpr const char* kJson = R"({"field1":"NaN"})";
    static constexpr DoubleMessageData kValue = {std::numeric_limits<double>::signaling_NaN()};
    static constexpr bool kSkip = !std::numeric_limits<double>::has_signaling_NaN;
};

struct DoublePositiveInfinity {
    using Message = proto_json::messages::DoubleMessage;
    static constexpr const char* kJson = R"({"field1":"Infinity"})";
    static constexpr DoubleMessageData kValue = {std::numeric_limits<double>::infinity()};
    static constexpr bool kSkip = !std::numeric_limits<double>::has_infinity;
};

struct DoubleNegativeInfinity {
    using Message = proto_json::messages::DoubleMessage;
    static constexpr const char* kJson = R"({"field1":"-Infinity"})";
    static constexpr DoubleMessageData kValue = {-std::numeric_limits<double>::infinity()};
    static constexpr bool kSkip = !std::numeric_limits<double>::has_infinity;
};

struct DoubleValue {
    using Message = ::google::protobuf::DoubleValue;
    static constexpr double kValue = 1.5;
    static constexpr const char* kJson = "1.5";
};

struct FloatValue {
    using Message = ::google::protobuf::FloatValue;
    static constexpr float kValue = 1.5;
    static constexpr const char* kJson = "1.5";
};

struct Int64Value {
    using Message = ::google::protobuf::Int64Value;
    static constexpr std::int64_t kValue = -123;
    static constexpr const char* kJson = "-123";
};

struct UInt64Value {
    using Message = ::google::protobuf::UInt64Value;
    static constexpr std::uint64_t kValue = 123;
    static constexpr const char* kJson = "123";
};

struct Int32Value {
    using Message = ::google::protobuf::Int32Value;
    static constexpr std::int32_t kValue = -321;
    static constexpr const char* kJson = "-321";
};

struct UInt32Value {
    using Message = ::google::protobuf::UInt32Value;
    static constexpr std::uint32_t kValue = 321;
    static constexpr const char* kJson = "321";
};

struct BoolValue {
    using Message = ::google::protobuf::BoolValue;
    static constexpr bool kValue = true;
    static constexpr const char* kJson = "true";
};

struct StringValue {
    using Message = ::google::protobuf::StringValue;
    static constexpr std::string_view kValue = "hello";
    static constexpr const char* kJson = "\"hello\"";
};

struct BytesValue {
    using Message = ::google::protobuf::BytesValue;
    static constexpr std::string_view kValue = "world";
    static constexpr const char* kJson = "\"d29ybGQ=\"";
};

template <typename TParam>
using WrappedType = std::conditional_t<
    !std::is_same_v<std::decay_t<decltype(TParam::kValue)>, std::string_view>,
    std::decay_t<decltype(TParam::kValue)>,
    std::string>;

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
