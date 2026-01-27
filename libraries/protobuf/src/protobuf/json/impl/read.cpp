#include <protobuf/json/impl/read.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/reflection.h>

#include <protobuf/json/impl/convert_utils.hpp>
#include <protobuf/json/impl/field_error.hpp>
#include <userver/crypto/base64.hpp>
#include <userver/formats/common/items.hpp>
#include <userver/protobuf/datetime.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/from_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::impl {

namespace {

enum class FindHint { kCamelCaseFirst = 1, kOriginalFirst = 2 };

void ReadGeneralMessage(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

void ReadAnyMessage(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

void ReadDurationMessage(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

void ReadTimestampMessage(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

void ReadFieldMaskMessage(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

void ReadValueMessage(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

void ReadListValueMessage(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

void ReadStructMessage(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

void ReadDoubleValueMessage(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

void ReadFloatValueMessage(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

void ReadInt64ValueMessage(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

void ReadUInt64ValueMessage(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

void ReadInt32ValueMessage(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

void ReadUInt32ValueMessage(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

void ReadBoolValueMessage(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

void ReadStringValueMessage(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

void ReadBytesValueMessage(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

using ReadMessageFunc = void (*)(const formats::json::Value&, ::google::protobuf::Message&, const ParseOptions&);

class SingularSetter {
public:
    static void SetInt64(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const std::int64_t value
    ) {
        reflection.SetInt64(&message, &field_desc, value);
    }

    static void SetUInt64(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const std::uint64_t value
    ) {
        reflection.SetUInt64(&message, &field_desc, value);
    }

    static void SetInt32(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const std::int32_t value
    ) {
        reflection.SetInt32(&message, &field_desc, value);
    }

    static void SetUInt32(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const std::uint32_t value
    ) {
        reflection.SetUInt32(&message, &field_desc, value);
    }

    static void SetBool(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const bool value
    ) {
        reflection.SetBool(&message, &field_desc, value);
    }

    static void SetFloat(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const float value
    ) {
        reflection.SetFloat(&message, &field_desc, value);
    }

    static void SetDouble(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const double value
    ) {
        reflection.SetDouble(&message, &field_desc, value);
    }

    static void SetString(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        std::string value
    ) {
        reflection.SetString(&message, &field_desc, std::move(value));
    }

    static void SetEnum(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const int value
    ) {
        reflection.SetEnumValue(&message, &field_desc, value);
    }

    [[nodiscard]] static ::google::protobuf::Message& NewMessage(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc
    ) {
        return *reflection.MutableMessage(&message, &field_desc);
    }
};

class RepeatedSetter {
public:
    static void SetInt64(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const std::int64_t value
    ) {
        reflection.AddInt64(&message, &field_desc, value);
    }

    static void SetUInt64(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const std::uint64_t value
    ) {
        reflection.AddUInt64(&message, &field_desc, value);
    }

    static void SetInt32(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const std::int32_t value
    ) {
        reflection.AddInt32(&message, &field_desc, value);
    }

    static void SetUInt32(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const std::uint32_t value
    ) {
        reflection.AddUInt32(&message, &field_desc, value);
    }

    static void SetBool(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const bool value
    ) {
        reflection.AddBool(&message, &field_desc, value);
    }

    static void SetFloat(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const float value
    ) {
        reflection.AddFloat(&message, &field_desc, value);
    }

    static void SetDouble(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const double value
    ) {
        reflection.AddDouble(&message, &field_desc, value);
    }

    static void SetString(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        std::string value
    ) {
        reflection.AddString(&message, &field_desc, std::move(value));
    }

    static void SetEnum(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const int value
    ) {
        reflection.AddEnumValue(&message, &field_desc, value);
    }

    [[nodiscard]] static ::google::protobuf::Message& NewMessage(
        ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc
    ) {
        return *reflection.AddMessage(&message, &field_desc);
    }
};

[[nodiscard]] const ::google::protobuf::FieldDescriptor* FindFieldByJsonName(
    const ::google::protobuf::Descriptor& desc,
    const std::string_view json_field_name,
    FindHint& hint
) {
    // according to ProtoJSON, conformant parser should accept both lowerCamelCase'-encoded
    // names and original protobuf message field names (usual lower_snake_case)
    const ::google::protobuf::FieldDescriptor* field_desc = nullptr;

    if (hint == FindHint::kCamelCaseFirst) {
        field_desc = desc.FindFieldByCamelcaseName(json_field_name);

        if (field_desc) {
            return field_desc;
        } else {
            field_desc = desc.FindFieldByName(json_field_name);

            if (field_desc) {
                hint = FindHint::kOriginalFirst;
                return field_desc;
            }
        }
    } else {
        field_desc = desc.FindFieldByName(json_field_name);

        if (field_desc) {
            return field_desc;
        } else {
            field_desc = desc.FindFieldByCamelcaseName(json_field_name);

            if (field_desc) {
                hint = FindHint::kCamelCaseFirst;
                return field_desc;
            }
        }
    }

    // field was not found using the basic algorithm, maybe `json_name` option was set for it

    for (int i = 0; i < desc.field_count(); ++i) {
        const auto* fdesc = desc.field(i);

        if (fdesc->has_json_name() && fdesc->json_name() == json_field_name) {
            field_desc = fdesc;
            break;
        }
    }

    return field_desc;
}

[[nodiscard]] ReadMessageFunc GetReadMessageFunc(const std::string_view full_name) {
    switch (ClassifyMessage(full_name)) {
        case MessageType::kGeneral:
            return ReadGeneralMessage;
        case MessageType::kAny:
            return ReadAnyMessage;
        case MessageType::kDuration:
            return ReadDurationMessage;
        case MessageType::kTimestamp:
            return ReadTimestampMessage;
        case MessageType::kFieldMask:
            return ReadFieldMaskMessage;
        case MessageType::kValue:
            return ReadValueMessage;
        case MessageType::kListValue:
            return ReadListValueMessage;
        case MessageType::kStruct:
            return ReadStructMessage;
        case MessageType::kDoubleValue:
            return ReadDoubleValueMessage;
        case MessageType::kFloatValue:
            return ReadFloatValueMessage;
        case MessageType::kInt64Value:
            return ReadInt64ValueMessage;
        case MessageType::kUInt64Value:
            return ReadUInt64ValueMessage;
        case MessageType::kInt32Value:
            return ReadInt32ValueMessage;
        case MessageType::kUInt32Value:
            return ReadUInt32ValueMessage;
        case MessageType::kBoolValue:
            return ReadBoolValueMessage;
        case MessageType::kStringValue:
            return ReadStringValueMessage;
        case MessageType::kBytesValue:
            return ReadBytesValueMessage;
    }

    UINVARIANT(false, "Protobuf message classifed to unknown type");
}

[[nodiscard]] bool StartsWith(const std::string_view& str, const std::string_view& prefix) {
    return std::string_view{str.data(), std::min(prefix.size(), str.size())} == prefix;
}

template <typename T>
bool ParseFloatStringAsInt(std::string_view str, T& value, double low, double high) {
    auto result = utils::FromStringNoThrow<double>(str);

    if (result) {
        const auto double_value = result.value();

        // checking that conversion will not overflow and the result is a whole number
        if (double_value >= low && double_value <= high) {
            value = static_cast<T>(double_value);
            return double_value - static_cast<double>(value) == 0;
        }
    }

    return false;
}

template <typename T, typename TString>
[[nodiscard]] T StringToNumber(const TString& str) {
    if constexpr (std ::is_floating_point_v<T>) {
        if (str == kNan) {
            return T{NAN};
        } else if (str == kPositiveInf) {
            return T{INFINITY};
        } else if (str == kNegativeInf) {
            return T{-INFINITY};
        }
    }

    auto result = utils::FromStringNoThrow<T>(str);

    if (result) {
        return result.value();
    } else if constexpr (std::is_integral_v<T>) {
        // ProtoJSON allows exponential format for integer numbers (for example, "1.8e3" is valid
        // according to specification), trying to parse string as double and use the result if
        // it is a whole number.

        if constexpr (std::is_signed_v<T>) {
            // min/max std::int64_t represented as double without precision loss
            constexpr double low = -9007199254740992.0;
            constexpr double high = 9007199254740992.0;
            std::int64_t value = 0;

            if (ParseFloatStringAsInt(str, value, low, high)) {
                if constexpr (std::is_same_v<T, std::int32_t>) {
                    if (value >= std::numeric_limits<std::int32_t>::min() &&
                        value <= std::numeric_limits<std::int32_t>::max())
                    {
                        return static_cast<std::int32_t>(value);
                    }
                } else {
                    return value;
                }
            }
        } else {
            // min/max std::uint64_t represented as double without precision loss
            constexpr double low = 0.0;
            constexpr double high = 18014398509481984.0;
            std::uint64_t value = 0;

            if (ParseFloatStringAsInt(str, value, low, high)) {
                if constexpr (std::is_same_v<T, std::uint32_t>) {
                    if (value >= std::numeric_limits<std::uint32_t>::min() &&
                        value <= std::numeric_limits<std::uint32_t>::max())
                    {
                        return static_cast<std::uint32_t>(value);
                    }
                } else {
                    return value;
                }
            }
        }
    }

    throw FieldError(ParseErrorCode::kInvalidValue);
}

[[nodiscard]] std::optional<int> StringToEnumValue(
    const std::string& str,
    const ::google::protobuf::EnumDescriptor& desc,
    const ParseOptions& options
) {
    auto value_desc = desc.FindValueByName(str);

    if (value_desc) {
        return value_desc->number();
    }

    // quoted number is also allowed for enum field, trying to parse string as number
    if (!str.empty() && (ascii_isdigit(str.front()) || str.front() == '+' || str.front() == '-')) {
        auto result = utils::FromStringNoThrow<std::int32_t>(str);

        if (result) {
            return result.value();
        } else {
            throw FieldError(ParseErrorCode::kInvalidValue);
        }
    }

    if (options.ignore_unknown_fields) {
        return std::nullopt;
    } else {
        throw FieldError(ParseErrorCode::kUnknownEnum);
    }
}

[[nodiscard]] std::uint32_t TakeTimeDigits(
    std::string_view& str,
    const std::size_t digit_count,
    const std::string_view prefix
) {
    if ((str.size() < digit_count + prefix.size()) || !StartsWith(str, prefix)) {
        throw FieldError(ParseErrorCode::kInvalidValue);
    }

    str.remove_prefix(prefix.size());
    std::uint32_t result = 0;

    for (std::size_t i = 0; i < digit_count; ++i) {
        if (ascii_isdigit(str[i])) {
            result *= 10;
            result += str[i] - '0';
        } else {
            throw FieldError(ParseErrorCode::kInvalidValue);
        }
    }

    str.remove_prefix(digit_count);
    return result;
}

[[nodiscard]] std::uint32_t TakeNanos(std::string_view& str) {
    std::uint32_t nanos = 0;
    std::size_t digit_count = 0;

    if (str.empty() || str.front() != '.') {
        return nanos;
    }

    str.remove_prefix(1);

    for (const char c : str) {
        if (ascii_isdigit(c)) {
            ++digit_count;
        } else {
            break;
        }
    }

    if (digit_count == 0 || digit_count > 9) {
        throw FieldError(ParseErrorCode::kInvalidValue);
    }

    nanos = StringToNumber<std::uint32_t>(str.substr(0, digit_count));
    str.remove_prefix(digit_count);

    for (std::size_t i = 0; i < 9 - digit_count; ++i) {
        nanos *= 10;
    }

    return nanos;
}

[[nodiscard]] std::int64_t CalcSecondsSinceEpoch(
    const std::uint32_t year,
    const std::uint32_t month,
    const std::uint32_t day,
    const std::uint32_t hours,
    const std::uint32_t minutes,
    const std::uint32_t seconds
) noexcept {
    // this is a standard efficient algorithm for calculating seconds since epoch,
    // implementation is copied from protobuf 22.5

    std::uint32_t adjusted_month = month - 3;  // March-based month
    std::uint32_t carry_flag = adjusted_month > month ? 1 : 0;

    std::uint32_t year_base = 4800;  // Before min year, multiple of 400.
    std::uint32_t adjusted_year = year + year_base - carry_flag;

    std::uint32_t month_days = ((adjusted_month + carry_flag * 12) * 62719 + 769) / 2048;
    std::uint32_t leap_days = adjusted_year / 4 - adjusted_year / 100 + adjusted_year / 400;
    std::int32_t epoch_days = adjusted_year * 365 + leap_days + month_days + (day - 1) - 2472632;

    return std::int64_t{epoch_days} * 86400 + hours * 3600 + minutes * 60 + seconds;
}

template <typename T>
[[nodiscard]] T ReadNumber(const formats::json::Value& value) {
    ParseErrorCode error_code = ParseErrorCode::kInvalidType;

    if (value.IsNumber()) {
        error_code = ParseErrorCode::kInvalidValue;

        if constexpr (std::is_same_v<T, std::int32_t>) {
            if (value.IsInt()) {
                return value.As<std::int32_t>();
            }
        } else if constexpr (std::is_same_v<T, std::uint32_t>) {
            if (value.IsUInt()) {
                return value.As<std::uint32_t>();
            }
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
            if (value.IsInt64()) {
                return value.As<std::int64_t>();
            }
        } else if constexpr (std::is_same_v<T, std::uint64_t>) {
            if (value.IsUInt64()) {
                return value.As<std::uint64_t>();
            }
        } else if constexpr (std::is_same_v<T, float>) {
            if (value.IsDouble()) {
                const auto val = value.As<double>();

                if (!std::isfinite(val) || std::isfinite(static_cast<float>(val))) {
                    // checking whether double can be converted to float without overflow
                    // explanation: non-finite double when casted is correctly represented with
                    // the same non-finite float while too big/small double will cause float
                    // to receive inf
                    return static_cast<float>(val);
                }
            }
        } else if constexpr (std::is_same_v<T, double>) {
            if (value.IsDouble()) {
                return value.As<double>();
            }
        } else {
            static_assert(sizeof(T) && false, "unexpected type");
        }
    } else if (value.IsString()) {
        const auto str = value.As<std::string>();
        return StringToNumber<T>(str);
    }

    throw FieldError(error_code);
}

[[nodiscard]] bool ReadBool(const formats::json::Value& value) {
    if (value.IsBool()) {
        return value.As<bool>();
    } else {
        throw FieldError(ParseErrorCode::kInvalidType);
    }
}

[[nodiscard]] std::string ReadString(const formats::json::Value& value) {
    if (value.IsString()) {
        return value.As<std::string>();
    } else {
        throw FieldError(ParseErrorCode::kInvalidType);
    }
}

[[nodiscard]] std::string ReadBytes(const formats::json::Value& value) {
    if (value.IsString()) {
        auto data = value.As<std::string>();

        if (crypto::base64::Base64UniversalDecodeInPlace(data)) {
            return data;
        } else {
            throw FieldError(ParseErrorCode::kInvalidValue);
        }
    } else {
        throw FieldError(ParseErrorCode::kInvalidType);
    }
}

[[nodiscard]] std::optional<int> ReadEnum(
    const formats::json::Value& value,
    const ::google::protobuf::EnumDescriptor& desc,
    const ParseOptions& options
) {
    if (value.IsString()) {
        const auto str = value.As<std::string>();
        return StringToEnumValue(str, desc, options);
    } else if (value.IsNumber()) {
        return ReadNumber<std::int32_t>(value);
    } else {
        throw FieldError(ParseErrorCode::kInvalidType);
    }
}

template <typename TSetter>
void ReadField(
    const formats::json::Value& json_field,
    ::google::protobuf::Message& message,
    const ::google::protobuf::Reflection& reflection,
    const ::google::protobuf::FieldDescriptor& field_desc,
    const ParseOptions& options,
    ReadMessageFunc& read_message
) {
    using ::google::protobuf::FieldDescriptor;

    UASSERT(message.GetReflection() == &reflection);
    UASSERT(field_desc.containing_type() == message.GetDescriptor());

    const auto type = field_desc.type();

    if (json_field.IsNull()) {
        // ProtoJSON has pretty cumbersome null handling logic:
        // 1. null for google.protobuf.NullValue type is treated as if field is explicitly
        //    set to 0 (NULL_VALUE), i.e. optional field of this type is considered set
        //    even if JSON contains null for it
        // 2. null for google.protobuf.Value type is treated as a value of it's 'null_value'
        //    field, which means that google.protobuf.Value field is considered set if JSON
        //    contains null for it
        // 3. null for all other singular field types is treated as absence of the field
        //    (fields with implicit presence will be default-initialized, hassers of a
        //    fields with explicit presence will return false)
        // 4. null inside repeated fields is prohibited for all cases except 1-2

        if (IsNullValue(field_desc)) {
            TSetter::SetEnum(message, reflection, field_desc, 0);
            return;
        }

        if (type == FieldDescriptor::TYPE_MESSAGE) {
            if (!read_message) {
                read_message = GetReadMessageFunc(field_desc.message_type()->full_name());
            }
        }

        if (read_message != &ReadValueMessage) {
            if (!field_desc.is_repeated() || field_desc.is_map()) {
                return;
            } else {
                throw FieldError(ParseErrorCode::kInvalidValue);
            }
        }
    }

    switch (type) {
        case FieldDescriptor::TYPE_INT64:
        case FieldDescriptor::TYPE_SFIXED64:
        case FieldDescriptor::TYPE_SINT64:
            TSetter::SetInt64(message, reflection, field_desc, ReadNumber<std::int64_t>(json_field));
            return;

        case FieldDescriptor::TYPE_UINT64:
        case FieldDescriptor::TYPE_FIXED64:
            TSetter::SetUInt64(message, reflection, field_desc, ReadNumber<std::uint64_t>(json_field));
            return;

        case FieldDescriptor::TYPE_INT32:
        case FieldDescriptor::TYPE_SFIXED32:
        case FieldDescriptor::TYPE_SINT32:
            TSetter::SetInt32(message, reflection, field_desc, ReadNumber<std::int32_t>(json_field));
            return;

        case FieldDescriptor::TYPE_UINT32:
        case FieldDescriptor::TYPE_FIXED32:
            TSetter::SetUInt32(message, reflection, field_desc, ReadNumber<std::uint32_t>(json_field));
            return;

        case FieldDescriptor::TYPE_BOOL:
            TSetter::SetBool(message, reflection, field_desc, ReadBool(json_field));
            return;

        case FieldDescriptor::TYPE_FLOAT:
            TSetter::SetFloat(message, reflection, field_desc, ReadNumber<float>(json_field));
            return;

        case FieldDescriptor::TYPE_DOUBLE:
            TSetter::SetDouble(message, reflection, field_desc, ReadNumber<double>(json_field));
            return;

        case FieldDescriptor::TYPE_STRING:
            TSetter::SetString(message, reflection, field_desc, ReadString(json_field));
            return;

        case FieldDescriptor::TYPE_BYTES:
            TSetter::SetString(message, reflection, field_desc, ReadBytes(json_field));
            return;

        case FieldDescriptor::TYPE_ENUM: {
            const auto value = ReadEnum(json_field, *field_desc.enum_type(), options);

            if (value.has_value() || !field_desc.has_presence()) {
                TSetter::SetEnum(message, reflection, field_desc, value.value_or(0));
            }

            return;
        }

        case FieldDescriptor::TYPE_GROUP:
        case FieldDescriptor::TYPE_MESSAGE: {
            auto& value = TSetter::NewMessage(message, reflection, field_desc);

            if (!read_message) {
                read_message = GetReadMessageFunc(field_desc.message_type()->full_name());
            }

            read_message(json_field, value, options);
            return;
        }
    }

    UINVARIANT(false, "Unexpected protobuf field descriptor type");
}

void ReadSingularField(
    const formats::json::Value& json_field,
    ::google::protobuf::Message& message,
    const ::google::protobuf::Reflection& reflection,
    const ::google::protobuf::FieldDescriptor& field_desc,
    const ParseOptions& options
) {
    UASSERT(message.GetReflection() == &reflection);
    UASSERT(field_desc.containing_type() == message.GetDescriptor());
    UASSERT(!field_desc.is_repeated());

    ReadMessageFunc read_message = nullptr;
    ReadField<SingularSetter>(json_field, message, reflection, field_desc, options, read_message);
}

void ReadRepeatedField(
    const formats::json::Value& json_field,
    ::google::protobuf::Message& message,
    const ::google::protobuf::Reflection& reflection,
    const ::google::protobuf::FieldDescriptor& field_desc,
    const ParseOptions& options
) {
    UASSERT(message.GetReflection() == &reflection);
    UASSERT(field_desc.containing_type() == message.GetDescriptor());
    UASSERT(field_desc.is_repeated());

    if (json_field.IsNull()) {
        return;
    }

    if (!json_field.IsArray()) {
        throw FieldError(ParseErrorCode::kInvalidType);
    }

    ReadMessageFunc read_message = nullptr;
    std::size_t index = 0;

    for (const auto& item : json_field) {
        try {
            ReadField<RepeatedSetter>(item, message, reflection, field_desc, options, read_message);
            ++index;
        } catch (FieldError& error) {
            error.PrependRepeatedIndex(index);
            throw;
        }
    }
}

void ReadMapField(
    const formats::json::Value& json_field,
    ::google::protobuf::Message& message,
    const ::google::protobuf::Reflection& reflection,
    const ::google::protobuf::FieldDescriptor& field_desc,
    const ParseOptions& options
) {
    using ::google::protobuf::FieldDescriptor;

    UASSERT(message.GetReflection() == &reflection);
    UASSERT(field_desc.containing_type() == message.GetDescriptor());
    UASSERT(field_desc.is_map());

    if (json_field.IsNull()) {
        return;
    }

    if (!json_field.IsObject()) {
        throw FieldError(ParseErrorCode::kInvalidType);
    }

    const FieldDescriptor& key_desc = *(field_desc.message_type()->map_key());
    const FieldDescriptor& value_desc = *(field_desc.message_type()->map_value());
    const auto key_type = key_desc.type();
    ReadMessageFunc read_message = nullptr;

    for (const auto& [name, value] : formats::common::Items(json_field)) {
        try {
            auto& item_message = *reflection.AddMessage(&message, &field_desc);
            const auto& item_reflection = *item_message.GetReflection();

            switch (key_type) {
                case FieldDescriptor::TYPE_INT64:
                case FieldDescriptor::TYPE_SFIXED64:
                case FieldDescriptor::TYPE_SINT64:
                    item_reflection.SetInt64(&item_message, &key_desc, StringToNumber<std::int64_t>(name));
                    break;

                case FieldDescriptor::TYPE_UINT64:
                case FieldDescriptor::TYPE_FIXED64:
                    item_reflection.SetUInt64(&item_message, &key_desc, StringToNumber<std::uint64_t>(name));
                    break;

                case FieldDescriptor::TYPE_INT32:
                case FieldDescriptor::TYPE_SFIXED32:
                case FieldDescriptor::TYPE_SINT32:
                    item_reflection.SetInt32(&item_message, &key_desc, StringToNumber<std::int32_t>(name));
                    break;

                case FieldDescriptor::TYPE_UINT32:
                case FieldDescriptor::TYPE_FIXED32:
                    item_reflection.SetUInt32(&item_message, &key_desc, StringToNumber<std::uint32_t>(name));
                    break;

                case FieldDescriptor::TYPE_BOOL: {
                    if (name == "true") {
                        item_reflection.SetBool(&item_message, &key_desc, true);
                    } else if (name == "false") {
                        item_reflection.SetBool(&item_message, &key_desc, false);
                    } else {
                        throw FieldError(ParseErrorCode::kInvalidValue);
                    }

                    break;
                }

                case FieldDescriptor::TYPE_STRING:
                    item_reflection.SetString(&item_message, &key_desc, std::string{name.begin(), name.end()});
                    break;

                default:
                    UINVARIANT(false, "Unexpected protobuf map key descriptor type");
            }

            ReadField<SingularSetter>(value, item_message, item_reflection, value_desc, options, read_message);
        } catch (FieldError& error) {
            error.PrependField(name);
            throw;
        }
    }
}

void ReadGeneralMessageImpl(
    const formats::json::Value& json,
    ::google::protobuf::Message& message,
    const ParseOptions& options,
    bool reading_any_payload = false
) {
    UASSERT(!json.IsMissing());

    if (!json.IsObject()) {
        throw FieldError(ParseErrorCode::kInvalidType);
    }

    const auto& desc = *message.GetDescriptor();
    const auto& reflection = *message.GetReflection();
    FindHint hint = FindHint::kCamelCaseFirst;
    std::unordered_set<int> read_oneof_indices;

    for (const auto& [name, value] : formats::common::Items(json)) {
        const auto field_desc = FindFieldByJsonName(desc, name, hint);

        try {
            if (!field_desc) {
                if (options.ignore_unknown_fields || (reading_any_payload && name == "@type")) {
                    continue;
                } else {
                    throw FieldError(ParseErrorCode::kUnknownField);
                }
            }

            if (field_desc->real_containing_oneof() && !value.IsNull()) {
                // checking that another oneof field is not read already
                if (!read_oneof_indices.insert(field_desc->real_containing_oneof()->index()).second) {
                    throw FieldError(ParseErrorCode::kMultipleOneofFields);
                }
            }

            if (!field_desc->is_repeated()) {
                ReadSingularField(value, message, reflection, *field_desc, options);
            } else {
                if (!field_desc->is_map()) {
                    ReadRepeatedField(value, message, reflection, *field_desc, options);
                } else {
                    ReadMapField(value, message, reflection, *field_desc, options);
                }
            }
        } catch (FieldError& error) {
            error.PrependField(name);
            throw;
        }
    }
}

void ReadGeneralMessage(
    const formats::json::Value& json,
    ::google::protobuf::Message& message,
    const ParseOptions& options
) {
    return ReadGeneralMessageImpl(json, message, options);
}

void ReadAnyMessage(
    const formats::json::Value& json,
    ::google::protobuf::Message& message,
    const ParseOptions& options
) {
    const auto& desc = *message.GetDescriptor();
    const auto& type_url_desc = GetMessageFieldDesc(desc, AnyTraits::kTypeUrlFieldNumber, AnyTraits::kTypeUrlFieldType);
    const auto& value_desc = GetMessageFieldDesc(desc, AnyTraits::kValueFieldNumber, AnyTraits::kValueFieldType);
    const auto& reflection = *message.GetReflection();

    if (!json.IsObject()) {
        throw FieldError(ParseErrorCode::kInvalidType);
    }

    if (json.IsEmpty()) {
        return;
    }

    if (!json.HasMember("@type")) {
        throw FieldError(ParseErrorCode::kInvalidValue);
    }

    const auto type_url_json = json["@type"];

    if (!type_url_json.IsString()) {
        // throwing 'kInvalidValue', not 'kInvalidType' because JSON field type here is
        // mapped to 'google.protobuf.Any' itself
        throw FieldError(ParseErrorCode::kInvalidValue);
    }

    std::string type_url = type_url_json.As<std::string>();
    const auto payload_desc = FindMessageDescByTypeUrl(*message.GetDescriptor()->file()->pool(), type_url);

    if (!payload_desc) {
        throw FieldError(ParseErrorCode::kInvalidValue);
    }

    ::google::protobuf::DynamicMessageFactory factory;
    const ReadMessageFunc read_message = GetReadMessageFunc(payload_desc->full_name());
    std::string payload;

    {
        // 'payload_message' should be deleted before 'factory' in case of exception
        std::unique_ptr<::google::protobuf::Message> payload_message(factory.GetPrototype(payload_desc)->New());

        if (read_message == &ReadGeneralMessage) {
            ReadGeneralMessageImpl(json, *payload_message, options, /*reading_any_payload=*/true);
        } else {
            std::size_t expected_field_count = 1;

            if (json.HasMember("value")) {
                const auto value_json = json["value"];
                read_message(value_json, *payload_message, options);
                ++expected_field_count;
            }

            if (json.GetSize() != expected_field_count && !options.ignore_unknown_fields) {
                // taking first unknown field
                for (const auto& [name, value] : formats::common::Items(json)) {
                    if (name != "@type" && name != "value") {
                        throw FieldError(ParseErrorCode::kUnknownField, name);
                    }
                }

                UINVARIANT(false, "Should not be here");
            }
        }

        payload = payload_message->SerializePartialAsString();
    }

    reflection.SetString(&message, &type_url_desc, std::move(type_url));
    reflection.SetString(&message, &value_desc, std::move(payload));
}

void ReadDurationMessage(const formats::json::Value& json, ::google::protobuf::Message& message, const ParseOptions&) {
    const auto& desc = *message.GetDescriptor();
    const auto& seconds_desc =
        GetMessageFieldDesc(desc, DurationTraits::kSecondsFieldNumber, DurationTraits::kSecondsFieldType);
    const auto&
        nanos_desc = GetMessageFieldDesc(desc, DurationTraits::kNanosFieldNumber, DurationTraits::kNanosFieldType);
    const auto& reflection = *message.GetReflection();

    if (json.IsNull()) {
        return;
    }

    auto storage = ReadString(json);
    std::string_view unparsed_str{storage};
    std::string_view seconds_str = unparsed_str;

    for (std::size_t i = 0; i < unparsed_str.size(); ++i) {
        if (!ascii_isdigit(unparsed_str[i]) && unparsed_str[i] != '-') {
            seconds_str = unparsed_str.substr(0, i);
            break;
        }
    }

    unparsed_str.remove_prefix(seconds_str.size());

    const auto seconds = StringToNumber<std::int64_t>(seconds_str);
    std::int32_t nanos = TakeNanos(unparsed_str);

    if (unparsed_str != "s") {
        throw FieldError(ParseErrorCode::kInvalidValue);
    }

    UASSERT(!seconds_str.empty());

    if (seconds_str.front() == '-') {
        // note that we can't use 'seconds < 0' because seconds may be 0
        nanos = -nanos;
    }

    if (IsValidDuration(seconds, nanos)) {
        reflection.SetInt64(&message, &seconds_desc, seconds);
        reflection.SetInt32(&message, &nanos_desc, nanos);
    } else {
        throw FieldError(ParseErrorCode::kInvalidValue);
    }
}

void ReadTimestampMessage(const formats::json::Value& json, ::google::protobuf::Message& message, const ParseOptions&) {
    const auto& desc = *message.GetDescriptor();
    const auto& seconds_desc =
        GetMessageFieldDesc(desc, TimestampTraits::kSecondsFieldNumber, TimestampTraits::kSecondsFieldType);
    const auto&
        nanos_desc = GetMessageFieldDesc(desc, TimestampTraits::kNanosFieldNumber, TimestampTraits::kNanosFieldType);
    const auto& reflection = *message.GetReflection();

    if (json.IsNull()) {
        return;
    }

    auto storage = ReadString(json);
    std::string_view unparsed_str{storage};

    // format 2019-05-28T23:00:00[.241300](Z|[+-]hh:mm)

    const auto year = TakeTimeDigits(unparsed_str, 4, "");
    const auto month = TakeTimeDigits(unparsed_str, 2, "-");
    const auto day = TakeTimeDigits(unparsed_str, 2, "-");
    const auto hours = TakeTimeDigits(unparsed_str, 2, "T");
    const auto minutes = TakeTimeDigits(unparsed_str, 2, ":");
    const auto seconds = TakeTimeDigits(unparsed_str, 2, ":");

    std::int64_t seconds_since_epoch = CalcSecondsSinceEpoch(year, month, day, hours, minutes, seconds);
    const std::int32_t nanos = TakeNanos(unparsed_str);

    if (unparsed_str.empty()) {
        throw FieldError(ParseErrorCode::kInvalidValue);
    }

    bool is_negative_offset = false;

    switch (unparsed_str.front()) {
        case '-':
            is_negative_offset = true;
            [[fallthrough]];
        case '+': {
            unparsed_str.remove_prefix(1);
            const auto tz_hours = TakeTimeDigits(unparsed_str, 2, "");
            const auto tz_minutes = TakeTimeDigits(unparsed_str, 2, ":");
            const std::int64_t tz_offset_seconds = (tz_hours * 60 + tz_minutes) * 60;
            seconds_since_epoch += (is_negative_offset ? tz_offset_seconds : -tz_offset_seconds);
            break;
        }
        case 'Z':
            unparsed_str.remove_prefix(1);
            break;
        default:
            break;
    }

    if (!unparsed_str.empty()) {
        throw FieldError(ParseErrorCode::kInvalidValue);
    }

    reflection.SetInt64(&message, &seconds_desc, seconds_since_epoch);
    reflection.SetInt32(&message, &nanos_desc, nanos);
}

void ReadFieldMaskMessage(const formats::json::Value& json, ::google::protobuf::Message& message, const ParseOptions&) {
    const auto& reflection = *message.GetReflection();
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        FieldMaskTraits::kPathsFieldNumber,
        FieldMaskTraits::kPathsFieldType,
        true
    );

    if (!json.IsString()) {
        throw FieldError(ParseErrorCode::kInvalidType);
    }

    const std::string paths = json.As<std::string>();

    if (paths.empty()) {
        return;
    }

    std::string path;
    path.reserve(paths.size() + 32);  // 32 bytes reserved for underscores

    for (const auto c : paths) {
        if (c == ',') {
            reflection.AddString(&message, &field_desc, std::move(path));
            path.clear();
        } else if (ascii_islower(c) || ascii_isdigit(c) || c == '.') {
            path.push_back(c);
        } else if (ascii_isupper(c)) {
            path.push_back('_');
            path.push_back(ascii_tolower(c));
        } else {
            throw FieldError(ParseErrorCode::kInvalidValue);
        }
    }

    reflection.AddString(&message, &field_desc, std::move(path));
}

void ReadValueMessage(
    const formats::json::Value& json,
    ::google::protobuf::Message& message,
    const ParseOptions& options
) {
    const auto& desc = *message.GetDescriptor();
    const auto& reflection = *message.GetReflection();

    if (json.IsNull()) {
        const auto& field_desc = GetMessageFieldDesc(desc, ValueTraits::kNullFieldNumber, ValueTraits::kNullFieldType);
        reflection.SetEnumValue(&message, &field_desc, 0);
    } else if (json.IsNumber()) {
        const auto&
            field_desc = GetMessageFieldDesc(desc, ValueTraits::kNumberFieldNumber, ValueTraits::kNumberFieldType);
        reflection.SetDouble(&message, &field_desc, json.As<double>());
    } else if (json.IsString()) {
        const auto&
            field_desc = GetMessageFieldDesc(desc, ValueTraits::kStringFieldNumber, ValueTraits::kStringFieldType);
        reflection.SetString(&message, &field_desc, json.As<std::string>());
    } else if (json.IsBool()) {
        const auto& field_desc = GetMessageFieldDesc(desc, ValueTraits::kBoolFieldNumber, ValueTraits::kBoolFieldType);
        reflection.SetBool(&message, &field_desc, json.As<bool>());
    } else if (json.IsObject()) {
        const auto&
            field_desc = GetMessageFieldDesc(desc, ValueTraits::kStructFieldNumber, ValueTraits::kStructFieldType);
        ReadStructMessage(json, *reflection.MutableMessage(&message, &field_desc), options);
    } else if (json.IsArray()) {
        const auto& field_desc = GetMessageFieldDesc(desc, ValueTraits::kListFieldNumber, ValueTraits::kListFieldType);
        ReadListValueMessage(json, *reflection.MutableMessage(&message, &field_desc), options);
    } else {
        UINVARIANT(false, "Unexpected JSON field type");
    }
}

void ReadListValueMessage(
    const formats::json::Value& json,
    ::google::protobuf::Message& message,
    const ParseOptions& options
) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        ListValueTraits::kValuesFieldNumber,
        ListValueTraits::kValuesFieldType,
        true
    );
    ReadRepeatedField(json, message, *message.GetReflection(), field_desc, options);
}

void ReadStructMessage(
    const formats::json::Value& json,
    ::google::protobuf::Message& message,
    const ParseOptions& options
) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        StructTraits::kFieldsFieldNumber,
        StructTraits::kFieldsFieldType,
        true
    );
    UINVARIANT(field_desc.is_map(), "Well-known message type 'google.protobuf.Struct' field 'fields' should be map");
    ReadMapField(json, message, *message.GetReflection(), field_desc, options);
}

void ReadDoubleValueMessage(const formats::json::Value& json, ::google::protobuf::Message& message, const ParseOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        DoubleValueTraits::kValueFieldNumber,
        DoubleValueTraits::kValueFieldType
    );

    if (json.IsNull()) {
        return;
    }

    message.GetReflection()->SetDouble(&message, &field_desc, ReadNumber<double>(json));
}

void ReadFloatValueMessage(const formats::json::Value& json, ::google::protobuf::Message& message, const ParseOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        FloatValueTraits::kValueFieldNumber,
        FloatValueTraits::kValueFieldType
    );

    if (json.IsNull()) {
        return;
    }

    message.GetReflection()->SetFloat(&message, &field_desc, ReadNumber<float>(json));
}

void ReadInt64ValueMessage(const formats::json::Value& json, ::google::protobuf::Message& message, const ParseOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        Int64ValueTraits::kValueFieldNumber,
        Int64ValueTraits::kValueFieldType
    );

    if (json.IsNull()) {
        return;
    }

    message.GetReflection()->SetInt64(&message, &field_desc, ReadNumber<std::int64_t>(json));
}

void ReadUInt64ValueMessage(const formats::json::Value& json, ::google::protobuf::Message& message, const ParseOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        UInt64ValueTraits::kValueFieldNumber,
        UInt64ValueTraits::kValueFieldType
    );

    if (json.IsNull()) {
        return;
    }

    message.GetReflection()->SetUInt64(&message, &field_desc, ReadNumber<std::uint64_t>(json));
}

void ReadInt32ValueMessage(const formats::json::Value& json, ::google::protobuf::Message& message, const ParseOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        Int32ValueTraits::kValueFieldNumber,
        Int32ValueTraits::kValueFieldType
    );

    if (json.IsNull()) {
        return;
    }

    message.GetReflection()->SetInt32(&message, &field_desc, ReadNumber<std::int32_t>(json));
}

void ReadUInt32ValueMessage(const formats::json::Value& json, ::google::protobuf::Message& message, const ParseOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        UInt32ValueTraits::kValueFieldNumber,
        UInt32ValueTraits::kValueFieldType
    );

    if (json.IsNull()) {
        return;
    }

    message.GetReflection()->SetUInt32(&message, &field_desc, ReadNumber<std::uint32_t>(json));
}

void ReadBoolValueMessage(const formats::json::Value& json, ::google::protobuf::Message& message, const ParseOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        BoolValueTraits::kValueFieldNumber,
        BoolValueTraits::kValueFieldType
    );

    if (json.IsNull()) {
        return;
    }

    message.GetReflection()->SetBool(&message, &field_desc, ReadBool(json));
}

void ReadStringValueMessage(const formats::json::Value& json, ::google::protobuf::Message& message, const ParseOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        StringValueTraits::kValueFieldNumber,
        StringValueTraits::kValueFieldType
    );

    if (json.IsNull()) {
        return;
    }

    message.GetReflection()->SetString(&message, &field_desc, ReadString(json));
}

void ReadBytesValueMessage(const formats::json::Value& json, ::google::protobuf::Message& message, const ParseOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        BytesValueTraits::kValueFieldNumber,
        BytesValueTraits::kValueFieldType
    );

    if (json.IsNull()) {
        return;
    }

    message.GetReflection()->SetString(&message, &field_desc, ReadBytes(json));
}

}  // namespace

void ReadMessage(
    const formats::json::Value& json,
    ::google::protobuf::Message& message,
    const ParseOptions& options
) try
{
    json.CheckNotMissing();
    message.Clear();
    const auto read = GetReadMessageFunc(message.GetDescriptor()->full_name());

    if (read == &ReadGeneralMessage && json.IsNull()) {
        // null JSON can be used only for some well-known types
        // ('google.protobuf.Duration', 'google.protobuf.Struct', etc.))
        throw FieldError(ParseErrorCode::kInvalidType);
    }

    read(json, message, options);
} catch (FieldError& error) {
    throw ParseError(error.GetCode<ParseErrorCode>(), std::move(error).GetPath());
}

}  // namespace protobuf::json::impl

USERVER_NAMESPACE_END
