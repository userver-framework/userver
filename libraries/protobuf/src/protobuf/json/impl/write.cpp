#include <protobuf/json/impl/write.hpp>

#include <cmath>
#include <string>
#include <string_view>

#include <fmt/format.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/reflection.h>

#include <protobuf/json/impl/convert_utils.hpp>
#include <protobuf/json/impl/field_error.hpp>
#include <userver/crypto/base64.hpp>
#include <userver/protobuf/datetime.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::impl {

namespace {

using ProtobufStringType =
    decltype(std::declval<::google::protobuf::Reflection>()
                 .GetString(std::declval<const ::google::protobuf::Message&>(), nullptr));

[[nodiscard]] formats::json::ValueBuilder WriteGeneralMessage(const ::google::protobuf::Message&, const PrintOptions&);

[[nodiscard]] formats::json::ValueBuilder WriteAnyMessage(const ::google::protobuf::Message&, const PrintOptions&);

[[nodiscard]] formats::json::ValueBuilder WriteDurationMessage(const ::google::protobuf::Message&, const PrintOptions&);

[[nodiscard]] formats::json::ValueBuilder
WriteTimestampMessage(const ::google::protobuf::Message&, const PrintOptions&);

[[nodiscard]] formats::json::ValueBuilder
WriteFieldMaskMessage(const ::google::protobuf::Message&, const PrintOptions&);

[[nodiscard]] formats::json::ValueBuilder WriteValueMessage(const ::google::protobuf::Message&, const PrintOptions&);

[[nodiscard]] formats::json::ValueBuilder
WriteListValueMessage(const ::google::protobuf::Message&, const PrintOptions&);

[[nodiscard]] formats::json::ValueBuilder WriteStructMessage(const ::google::protobuf::Message&, const PrintOptions&);

[[nodiscard]] formats::json::ValueBuilder
WriteDoubleValueMessage(const ::google::protobuf::Message&, const PrintOptions&);

[[nodiscard]] formats::json::ValueBuilder
WriteFloatValueMessage(const ::google::protobuf::Message&, const PrintOptions&);

[[nodiscard]] formats::json::ValueBuilder
WriteInt64ValueMessage(const ::google::protobuf::Message&, const PrintOptions&);

[[nodiscard]] formats::json::ValueBuilder
WriteUInt64ValueMessage(const ::google::protobuf::Message&, const PrintOptions&);

[[nodiscard]] formats::json::ValueBuilder
WriteInt32ValueMessage(const ::google::protobuf::Message&, const PrintOptions&);

[[nodiscard]] formats::json::ValueBuilder
WriteUInt32ValueMessage(const ::google::protobuf::Message&, const PrintOptions&);

[[nodiscard]] formats::json::ValueBuilder
WriteBoolValueMessage(const ::google::protobuf::Message&, const PrintOptions&);

[[nodiscard]] formats::json::ValueBuilder
WriteStringValueMessage(const ::google::protobuf::Message&, const PrintOptions&);

[[nodiscard]] formats::json::ValueBuilder
WriteBytesValueMessage(const ::google::protobuf::Message&, const PrintOptions&);

using WriteMessageFunc = formats::json::ValueBuilder (*)(const ::google::protobuf::Message&, const PrintOptions&);

class SingularGetter {
public:
    [[nodiscard]] static std::int64_t GetInt64(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        int
    ) {
        return reflection.GetInt64(message, &field_desc);
    }

    [[nodiscard]] static std::uint64_t GetUInt64(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        int
    ) {
        return reflection.GetUInt64(message, &field_desc);
    }

    [[nodiscard]] static std::int32_t GetInt32(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        int
    ) {
        return reflection.GetInt32(message, &field_desc);
    }

    [[nodiscard]] static std::uint32_t GetUInt32(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        int
    ) {
        return reflection.GetUInt32(message, &field_desc);
    }

    [[nodiscard]] static bool GetBool(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        int
    ) {
        return reflection.GetBool(message, &field_desc);
    }

    [[nodiscard]] static float GetFloat(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        int
    ) {
        return reflection.GetFloat(message, &field_desc);
    }

    [[nodiscard]] static double GetDouble(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        int
    ) {
        return reflection.GetDouble(message, &field_desc);
    }

    [[nodiscard]] static const ProtobufStringType& GetString(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        int,
        ProtobufStringType& scratch
    ) {
        return reflection.GetStringReference(message, &field_desc, &scratch);
    }

    [[nodiscard]] static const ::google::protobuf::EnumValueDescriptor& GetEnum(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        int
    ) {
        return *reflection.GetEnum(message, &field_desc);
    }

    [[nodiscard]] static const ::google::protobuf::Message& GetMessage(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        int
    ) {
        return reflection.GetMessage(message, &field_desc);
    }
};

class RepeatedGetter {
public:
    [[nodiscard]] static std::int64_t GetInt64(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const int index
    ) {
        return reflection.GetRepeatedInt64(message, &field_desc, index);
    }

    [[nodiscard]] static std::uint64_t GetUInt64(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const int index
    ) {
        return reflection.GetRepeatedUInt64(message, &field_desc, index);
    }

    [[nodiscard]] static std::int32_t GetInt32(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const int index
    ) {
        return reflection.GetRepeatedInt32(message, &field_desc, index);
    }

    [[nodiscard]] static std::uint32_t GetUInt32(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const int index
    ) {
        return reflection.GetRepeatedUInt32(message, &field_desc, index);
    }

    [[nodiscard]] static bool GetBool(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const int index
    ) {
        return reflection.GetRepeatedBool(message, &field_desc, index);
    }

    [[nodiscard]] static float GetFloat(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const int index
    ) {
        return reflection.GetRepeatedFloat(message, &field_desc, index);
    }

    [[nodiscard]] static double GetDouble(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const int index
    ) {
        return reflection.GetRepeatedDouble(message, &field_desc, index);
    }

    [[nodiscard]] static const ProtobufStringType& GetString(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const int index,
        ProtobufStringType& scratch
    ) {
        return reflection.GetRepeatedStringReference(message, &field_desc, index, &scratch);
    }

    [[nodiscard]] static const ::google::protobuf::EnumValueDescriptor& GetEnum(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const int index
    ) {
        return *reflection.GetRepeatedEnum(message, &field_desc, index);
    }

    [[nodiscard]] static const ::google::protobuf::Message& GetMessage(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        const int index
    ) {
        return reflection.GetRepeatedMessage(message, &field_desc, index);
    }
};

[[nodiscard]] WriteMessageFunc GetWriteMessageFunc(const std::string_view full_name) {
    switch (ClassifyMessage(full_name)) {
        case MessageType::kGeneral:
            return WriteGeneralMessage;
        case MessageType::kAny:
            return WriteAnyMessage;
        case MessageType::kDuration:
            return WriteDurationMessage;
        case MessageType::kTimestamp:
            return WriteTimestampMessage;
        case MessageType::kFieldMask:
            return WriteFieldMaskMessage;
        case MessageType::kValue:
            return WriteValueMessage;
        case MessageType::kListValue:
            return WriteListValueMessage;
        case MessageType::kStruct:
            return WriteStructMessage;
        case MessageType::kDoubleValue:
            return WriteDoubleValueMessage;
        case MessageType::kFloatValue:
            return WriteFloatValueMessage;
        case MessageType::kInt64Value:
            return WriteInt64ValueMessage;
        case MessageType::kUInt64Value:
            return WriteUInt64ValueMessage;
        case MessageType::kInt32Value:
            return WriteInt32ValueMessage;
        case MessageType::kUInt32Value:
            return WriteUInt32ValueMessage;
        case MessageType::kBoolValue:
            return WriteBoolValueMessage;
        case MessageType::kStringValue:
            return WriteStringValueMessage;
        case MessageType::kBytesValue:
            return WriteBytesValueMessage;
    }

    UINVARIANT(false, "Protobuf message classifed to unknown type");
}

template <typename T>
[[nodiscard]] formats::json::ValueBuilder GetFloatJsonValue(const T value) {
    if (std::isnan(value)) {
        return formats::json::ValueBuilder{kNan};
    } else if (std::isinf(value)) {
        return formats::json::ValueBuilder{value < 0 ? kNegativeInf : kPositiveInf};
    } else {
        return formats::json::ValueBuilder{value};
    }
}

template <typename TReader>
[[nodiscard]] formats::json::ValueBuilder WriteField(
    const ::google::protobuf::Message& message,
    const ::google::protobuf::Reflection& reflection,
    const ::google::protobuf::FieldDescriptor& field_desc,
    const PrintOptions& options,
    WriteMessageFunc& write_message,
    const int index = -1
) {
    using ::google::protobuf::FieldDescriptor;

    UASSERT(message.GetReflection() == &reflection);
    UASSERT(field_desc.containing_type() == message.GetDescriptor());
    UASSERT((field_desc.is_repeated() && index >= 0) || (!field_desc.is_repeated() && index < 0));

    const auto type = field_desc.type();

    switch (type) {
        case FieldDescriptor::TYPE_INT64:
        case FieldDescriptor::TYPE_SFIXED64:
        case FieldDescriptor::TYPE_SINT64:
            return formats::json::ValueBuilder{std::to_string(TReader::GetInt64(message, reflection, field_desc, index))
            };

        case FieldDescriptor::TYPE_UINT64:
        case FieldDescriptor::TYPE_FIXED64:
            return formats::json::ValueBuilder{std::to_string(TReader::GetUInt64(message, reflection, field_desc, index)
            )};

        case FieldDescriptor::TYPE_INT32:
        case FieldDescriptor::TYPE_SFIXED32:
        case FieldDescriptor::TYPE_SINT32:
            return formats::json::ValueBuilder{TReader::GetInt32(message, reflection, field_desc, index)};

        case FieldDescriptor::TYPE_UINT32:
        case FieldDescriptor::TYPE_FIXED32:
            return formats::json::ValueBuilder{TReader::GetUInt32(message, reflection, field_desc, index)};

        case FieldDescriptor::TYPE_BOOL:
            return formats::json::ValueBuilder{TReader::GetBool(message, reflection, field_desc, index)};

        case FieldDescriptor::TYPE_FLOAT:
            return GetFloatJsonValue(TReader::GetFloat(message, reflection, field_desc, index));

        case FieldDescriptor::TYPE_DOUBLE:
            return GetFloatJsonValue(TReader::GetDouble(message, reflection, field_desc, index));

        case FieldDescriptor::TYPE_STRING: {
            ProtobufStringType scratch;
            return formats::json::ValueBuilder{
                std::string_view(TReader::GetString(message, reflection, field_desc, index, scratch))
            };
        }

        case FieldDescriptor::TYPE_BYTES: {
            ProtobufStringType scratch;
            std::string str = TReader::GetString(message, reflection, field_desc, index, scratch);
            return formats::json::ValueBuilder{crypto::base64::Base64Encode(str)};
        }

        case FieldDescriptor::TYPE_ENUM: {
            const auto& enum_value_desc = TReader::GetEnum(message, reflection, field_desc, index);

            if (!IsNullValue(*enum_value_desc.type())) {
                return options.always_print_enums_as_ints
                           ? formats::json::ValueBuilder{enum_value_desc.number()}
                           : formats::json::ValueBuilder{std::string_view{enum_value_desc.name()}};
            } else {
                // 'google.protobuf.NullValue' enum is used to represent null in the JSON
                // (not only as part of the 'google.protobuf.Value' but also on it's own)
                return formats::json::ValueBuilder{};
            }
        }

        case FieldDescriptor::TYPE_GROUP:
        case FieldDescriptor::TYPE_MESSAGE: {
            const auto& value = TReader::GetMessage(message, reflection, field_desc, index);

            if (!write_message) {
                write_message = GetWriteMessageFunc(field_desc.message_type()->full_name());
            }

            return write_message(value, options);
        }
    }

    UINVARIANT(false, "Unexpected protobuf field descriptor type");
}

[[nodiscard]] formats::json::ValueBuilder WriteSingularField(
    const ::google::protobuf::Message& message,
    const ::google::protobuf::Reflection& reflection,
    const ::google::protobuf::FieldDescriptor& field_desc,
    const PrintOptions& options
) {
    UASSERT(message.GetReflection() == &reflection);
    UASSERT(field_desc.containing_type() == message.GetDescriptor());
    UASSERT(!field_desc.is_repeated());

    try {
        WriteMessageFunc write_message = nullptr;
        return WriteField<SingularGetter>(message, reflection, field_desc, options, write_message);
    } catch (FieldError& error) {
        error.PrependField(field_desc.name());
        throw;
    }
}

[[nodiscard]] formats::json::ValueBuilder WriteRepeatedField(
    const ::google::protobuf::Message& message,
    const ::google::protobuf::Reflection& reflection,
    const ::google::protobuf::FieldDescriptor& field_desc,
    const PrintOptions& options
) {
    UASSERT(message.GetReflection() == &reflection);
    UASSERT(field_desc.containing_type() == message.GetDescriptor());
    UASSERT(field_desc.is_repeated());

    formats::json::ValueBuilder array{formats::common::Type::kArray};
    const auto size = reflection.FieldSize(message, &field_desc);
    WriteMessageFunc write_message = nullptr;

    for (int i = 0; i < size; ++i) {
        try {
            array.PushBack(WriteField<RepeatedGetter>(message, reflection, field_desc, options, write_message, i));
        } catch (FieldError& error) {
            error.PrependRepeatedItem(field_desc.name(), i);
            throw;
        }
    }

    return array;
}

[[nodiscard]] formats::json::ValueBuilder WriteMapField(
    const ::google::protobuf::Message& message,
    const ::google::protobuf::Reflection& reflection,
    const ::google::protobuf::FieldDescriptor& field_desc,
    const PrintOptions& options
) {
    using ::google::protobuf::FieldDescriptor;

    UASSERT(message.GetReflection() == &reflection);
    UASSERT(field_desc.containing_type() == message.GetDescriptor());
    UASSERT(field_desc.is_map());
    UASSERT(field_desc.message_type() != nullptr);

    formats::json::ValueBuilder object{formats::common::Type::kObject};

    const FieldDescriptor& key_desc = *(field_desc.message_type()->map_key());
    const FieldDescriptor& value_desc = *(field_desc.message_type()->map_value());
    const auto key_type = key_desc.type();
    const auto size = reflection.FieldSize(message, &field_desc);
    WriteMessageFunc write_message = nullptr;

    for (int i = 0; i < size; ++i) {
        const auto& item_message = reflection.GetRepeatedMessage(message, &field_desc, i);
        const auto& item_reflection = *item_message.GetReflection();
        std::string key;

        switch (key_type) {
            case FieldDescriptor::TYPE_INT64:
            case FieldDescriptor::TYPE_SFIXED64:
            case FieldDescriptor::TYPE_SINT64:
                key = std::to_string(item_reflection.GetInt64(item_message, &key_desc));
                break;

            case FieldDescriptor::TYPE_UINT64:
            case FieldDescriptor::TYPE_FIXED64:
                key = std::to_string(item_reflection.GetUInt64(item_message, &key_desc));
                break;

            case FieldDescriptor::TYPE_INT32:
            case FieldDescriptor::TYPE_SFIXED32:
            case FieldDescriptor::TYPE_SINT32:
                key = std::to_string(item_reflection.GetInt32(item_message, &key_desc));
                break;

            case FieldDescriptor::TYPE_UINT32:
            case FieldDescriptor::TYPE_FIXED32:
                key = std::to_string(item_reflection.GetUInt32(item_message, &key_desc));
                break;

            case FieldDescriptor::TYPE_BOOL:
                key = item_reflection.GetBool(item_message, &key_desc) ? "true" : "false";
                break;

            case FieldDescriptor::TYPE_STRING:
                key = item_reflection.GetString(item_message, &key_desc);
                break;

            default:
                UINVARIANT(false, "Unexpected protobuf map key descriptor type");
        }

        try {
            object[std::move(key
            )] = WriteField<SingularGetter>(item_message, item_reflection, value_desc, options, write_message);
        } catch (FieldError& error) {
            error.PrependMapItem(field_desc.name(), key);
            throw;
        }
    }

    return object;
}

formats::json::ValueBuilder WriteGeneralMessage(
    const ::google::protobuf::Message& message,
    const PrintOptions& options
) {
    formats::json::ValueBuilder json(formats::common::Type::kObject);
    const auto& desc = *message.GetDescriptor();
    const auto& reflection = *message.GetReflection();

    for (int i = 0; i < desc.field_count(); ++i) {
        const auto& field_desc = *desc.field(i);
        const auto has_field =
            !field_desc.is_repeated()
                ? reflection.HasField(message, &field_desc)  // || IsNullValue(field_desc))
                : (reflection.FieldSize(message, &field_desc) != 0);

        if (has_field || (options.always_print_fields_with_no_presence && !field_desc.has_presence())) {
            std::string_view
                field_name = options.preserve_proto_field_names ? field_desc.name() : field_desc.json_name();

            if (!field_desc.is_repeated()) {
                json.EmplaceNocheck(field_name, WriteSingularField(message, reflection, field_desc, options));
            } else {
                if (!field_desc.is_map()) {
                    json.EmplaceNocheck(field_name, WriteRepeatedField(message, reflection, field_desc, options));
                } else {
                    json.EmplaceNocheck(field_name, WriteMapField(message, reflection, field_desc, options));
                }
            }
        }
    }

    return json;
}

formats::json::ValueBuilder WriteAnyMessage(const ::google::protobuf::Message& message, const PrintOptions& options) {
    const auto& desc = *message.GetDescriptor();
    const auto& type_url_desc = GetMessageFieldDesc(desc, AnyTraits::kTypeUrlFieldNumber, AnyTraits::kTypeUrlFieldType);
    const auto& value_desc = GetMessageFieldDesc(desc, AnyTraits::kValueFieldNumber, AnyTraits::kValueFieldType);
    const auto& reflection = *message.GetReflection();

    using StringType = decltype(reflection.GetString(message, &type_url_desc));

    StringType scratch1, scratch2;
    const auto& type_url = reflection.GetStringReference(message, &type_url_desc, &scratch1);
    const auto& value = reflection.GetStringReference(message, &value_desc, &scratch2);

    formats::json::ValueBuilder object{formats::common::Type::kObject};

    if (type_url.empty() && value.empty()) {
        return object;
    }

    const auto payload_desc = FindMessageDescByTypeUrl(*message.GetDescriptor()->file()->pool(), type_url);

    if (!payload_desc) {
        throw FieldError(PrintErrorCode::kInvalidValue);
    }

    ::google::protobuf::DynamicMessageFactory factory;

    {
        // 'payload_message' should be deleted before 'factory' in case of exception
        std::unique_ptr<::google::protobuf::Message> payload_message(factory.GetPrototype(payload_desc)->New());

        if (!payload_message->ParsePartialFromString(value)) {
            throw FieldError(PrintErrorCode::kInvalidValue);
        }

        const WriteMessageFunc write_message = GetWriteMessageFunc(payload_desc->full_name());

        if (write_message == &WriteGeneralMessage) {
            object = write_message(*payload_message, options);
        } else {
            object["value"] = write_message(*payload_message, options);
        }

        object["@type"] = std::string_view{type_url};
    }

    return object;
}

formats::json::ValueBuilder WriteDurationMessage(const ::google::protobuf::Message& message, const PrintOptions&) {
    const auto& desc = *message.GetDescriptor();
    const auto& seconds_desc =
        GetMessageFieldDesc(desc, DurationTraits::kSecondsFieldNumber, DurationTraits::kSecondsFieldType);
    const auto&
        nanos_desc = GetMessageFieldDesc(desc, DurationTraits::kNanosFieldNumber, DurationTraits::kNanosFieldType);
    const auto& reflection = *message.GetReflection();

    const auto seconds = reflection.GetInt64(message, &seconds_desc);
    const auto nanos = reflection.GetInt32(message, &nanos_desc);

    if (!IsValidDuration(seconds, nanos)) {
        throw FieldError(PrintErrorCode::kInvalidValue);
    }

    std::string value;

    if (nanos == 0) {
        value = fmt::format("{}s", seconds);
    } else {
        std::int32_t seconds_fraction = std::abs(nanos);
        int digits = 9;

        while (seconds_fraction % 1000 == 0) {
            seconds_fraction /= 1000;
            digits -= 3;
        }

        const std::string_view sign = (seconds < 0 || nanos < 0) ? "-" : "";
        value = fmt::format("{}{}.{:0{}}s", sign, std::abs(seconds), seconds_fraction, digits);
    }

    return formats::json::ValueBuilder{std::move(value)};
}

formats::json::ValueBuilder WriteTimestampMessage(const ::google::protobuf::Message& message, const PrintOptions&) {
    const auto& desc = *message.GetDescriptor();
    const auto& seconds_desc =
        GetMessageFieldDesc(desc, TimestampTraits::kSecondsFieldNumber, TimestampTraits::kSecondsFieldType);
    const auto&
        nanos_desc = GetMessageFieldDesc(desc, TimestampTraits::kNanosFieldNumber, TimestampTraits::kNanosFieldType);
    const auto& reflection = *message.GetReflection();

    auto seconds = reflection.GetInt64(message, &seconds_desc);
    const auto nanos = reflection.GetInt32(message, &nanos_desc);

    if (!IsValidTimestamp(seconds, nanos)) {
        throw FieldError(PrintErrorCode::kInvalidValue);
    }

    // ensure that seconds is positive (kMinTimestampSeconds is negative)
    seconds -= kMinTimestampSeconds;

    // Julian Day -> Y/M/D, Algorithm from:
    // Fliegel, H. F., and Van Flandern, T. C., "A Machine Algorithm for Processing Calendar Dates,"
    // Communications of the Association of Computing Machines, vol. 11 (1968), p. 657.
    // Implementation is taken from protobuf sources:
    // https://github.com/protocolbuffers/protobuf/blob/v33.2/src/google/protobuf/json/internal/unparser.cc#L612

    // NOLINTNEXTLINE(readability-identifier-naming)
    std::int32_t L = 0, N = 0, I = 0, J = 0, K = 0;
    L = static_cast<std::int32_t>(seconds / 86400) - 719162 + 68569 + 2440588;
    N = 4 * L / 146097;
    L = L - (146097 * N + 3) / 4;
    I = 4000 * (L + 1) / 1461001;
    L = L - 1461 * I / 4 + 31;
    J = 80 * L / 2447;
    K = L - 2447 * J / 80;
    L = J / 11;
    J = J + 2 - 12 * L;
    I = 100 * (N - 49) + I + L;

    const std::int32_t sec = seconds % 60;
    const std::int32_t min = (seconds / 60) % 60;
    const std::int32_t hour = (seconds / 3600) % 24;

    std::string value;

    if (nanos == 0) {
        value = fmt::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}Z", I, J, K, hour, min, sec);
    } else {
        std::int32_t seconds_fraction = nanos;  // nanos greater than 0
        int digits = 9;

        while (seconds_fraction % 1000 == 0) {
            seconds_fraction /= 1000;
            digits -= 3;
        }

        value = fmt::format(
            "{:04}-{:02}-{:02}T{:02}:{:02}:{:02}.{:0{}}Z",
            I,
            J,
            K,
            hour,
            min,
            sec,
            seconds_fraction,
            digits
        );
    }

    return formats::json::ValueBuilder{std::move(value)};
}

formats::json::ValueBuilder WriteFieldMaskMessage(const ::google::protobuf::Message& message, const PrintOptions&) {
    std::string json_paths;
    const auto& reflection = *message.GetReflection();
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        FieldMaskTraits::kPathsFieldNumber,
        FieldMaskTraits::kPathsFieldType,
        true
    );
    const int size = reflection.FieldSize(message, &field_desc);

    using StringType = decltype(reflection.GetString(message, &field_desc));

    for (int i = 0; i < size; ++i) {
        StringType scratch;
        std::string_view path = reflection.GetRepeatedStringReference(message, &field_desc, i, &scratch);

        std::string json_path;
        json_path.reserve(path.size());

        bool underscore_seen = false;

        for (const char c : path) {
            if (ascii_islower(c)) {
                if (!underscore_seen) {
                    json_path.push_back(c);
                } else {
                    json_path.push_back(ascii_toupper(c));
                }
            } else if (c == '.' || (ascii_isdigit(c) && !underscore_seen)) {
                // parser will not be able to restore original path if it contains digit after underscore
                json_path.push_back(c);
            } else if (c == '_' && !underscore_seen) {
                underscore_seen = true;
                continue;
            } else {
                throw FieldError(PrintErrorCode::kInvalidValue);
            }

            underscore_seen = false;
        }

        if (underscore_seen) {
            // parser will not be able to restore original path if it contains trailing underscore
            throw FieldError(PrintErrorCode::kInvalidValue);
        }

        json_paths.append(json_path);
        json_paths.append(1, ',');
    }

    if (!json_paths.empty()) {
        json_paths.pop_back();  // remove trailing ','
    }

    return formats::json::ValueBuilder{std::move(json_paths)};
}

formats::json::ValueBuilder WriteValueMessage(const ::google::protobuf::Message& message, const PrintOptions& options) {
    const auto& desc = *message.GetDescriptor();
    const auto& reflection = *message.GetReflection();

    {
        const auto& field_desc = GetMessageFieldDesc(desc, ValueTraits::kNullFieldNumber, ValueTraits::kNullFieldType);

        if (reflection.HasField(message, &field_desc)) {
            return formats::json::ValueBuilder{formats::common::Type::kNull};
        }
    }

    {
        const auto&
            field_desc = GetMessageFieldDesc(desc, ValueTraits::kNumberFieldNumber, ValueTraits::kNumberFieldType);

        if (reflection.HasField(message, &field_desc)) {
            const auto value = reflection.GetDouble(message, &field_desc);

            if (std::isnan(value) || std::isinf(value)) {
                // not supported for google.protobuf.Value (would be represented as string in JSON)
                throw FieldError(PrintErrorCode::kInvalidValue, field_desc.name());
            }

            return formats::json::ValueBuilder{value};
        }
    }

    {
        const auto&
            field_desc = GetMessageFieldDesc(desc, ValueTraits::kStringFieldNumber, ValueTraits::kStringFieldType);

        if (reflection.HasField(message, &field_desc)) {
            using StringType = decltype(reflection.GetString(message, &field_desc));

            StringType scratch;
            std::string_view str = reflection.GetStringReference(message, &field_desc, &scratch);
            return formats::json::ValueBuilder{str};
        }
    }

    {
        const auto& field_desc = GetMessageFieldDesc(desc, ValueTraits::kBoolFieldNumber, ValueTraits::kBoolFieldType);

        if (reflection.HasField(message, &field_desc)) {
            return formats::json::ValueBuilder{reflection.GetBool(message, &field_desc)};
        }
    }

    {
        const auto&
            field_desc = GetMessageFieldDesc(desc, ValueTraits::kStructFieldNumber, ValueTraits::kStructFieldType);

        if (reflection.HasField(message, &field_desc)) {
            try {
                return WriteStructMessage(reflection.GetMessage(message, &field_desc), options);
            } catch (FieldError& error) {
                error.PrependField("struct_value");
                throw;
            }
        }
    }

    {
        const auto& field_desc = GetMessageFieldDesc(desc, ValueTraits::kListFieldNumber, ValueTraits::kListFieldType);

        if (reflection.HasField(message, &field_desc)) {
            try {
                return WriteListValueMessage(reflection.GetMessage(message, &field_desc), options);
            } catch (FieldError& error) {
                error.PrependField("list_value");
                throw;
            }
        }
    }

    // one of the Value's oneof field must be set
    throw FieldError(PrintErrorCode::kInvalidValue);
}

formats::json::ValueBuilder WriteListValueMessage(
    const ::google::protobuf::Message& message,
    const PrintOptions& options
) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        ListValueTraits::kValuesFieldNumber,
        ListValueTraits::kValuesFieldType,
        true
    );
    return WriteRepeatedField(message, *message.GetReflection(), field_desc, options);
}

formats::json::ValueBuilder WriteStructMessage(
    const ::google::protobuf::Message& message,
    const PrintOptions& options
) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        StructTraits::kFieldsFieldNumber,
        StructTraits::kFieldsFieldType,
        true
    );
    UINVARIANT(field_desc.is_map(), "Well-known message type 'google.protobuf.Struct' field 'fields' should be map");
    return WriteMapField(message, *message.GetReflection(), field_desc, options);
}

formats::json::ValueBuilder WriteDoubleValueMessage(const ::google::protobuf::Message& message, const PrintOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        DoubleValueTraits::kValueFieldNumber,
        DoubleValueTraits::kValueFieldType
    );
    return GetFloatJsonValue(message.GetReflection()->GetDouble(message, &field_desc));
}

formats::json::ValueBuilder WriteFloatValueMessage(const ::google::protobuf::Message& message, const PrintOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        FloatValueTraits::kValueFieldNumber,
        FloatValueTraits::kValueFieldType
    );
    return GetFloatJsonValue(message.GetReflection()->GetFloat(message, &field_desc));
}

formats::json::ValueBuilder WriteInt64ValueMessage(const ::google::protobuf::Message& message, const PrintOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        Int64ValueTraits::kValueFieldNumber,
        Int64ValueTraits::kValueFieldType
    );
    return formats::json::ValueBuilder{std::to_string(message.GetReflection()->GetInt64(message, &field_desc))};
}

formats::json::ValueBuilder WriteUInt64ValueMessage(const ::google::protobuf::Message& message, const PrintOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        UInt64ValueTraits::kValueFieldNumber,
        UInt64ValueTraits::kValueFieldType
    );
    return formats::json::ValueBuilder{std::to_string(message.GetReflection()->GetUInt64(message, &field_desc))};
}

formats::json::ValueBuilder WriteInt32ValueMessage(const ::google::protobuf::Message& message, const PrintOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        Int32ValueTraits::kValueFieldNumber,
        Int32ValueTraits::kValueFieldType
    );
    return formats::json::ValueBuilder{message.GetReflection()->GetInt32(message, &field_desc)};
}

formats::json::ValueBuilder WriteUInt32ValueMessage(const ::google::protobuf::Message& message, const PrintOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        UInt32ValueTraits::kValueFieldNumber,
        UInt32ValueTraits::kValueFieldType
    );
    return formats::json::ValueBuilder{message.GetReflection()->GetUInt32(message, &field_desc)};
}

formats::json::ValueBuilder WriteBoolValueMessage(const ::google::protobuf::Message& message, const PrintOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        BoolValueTraits::kValueFieldNumber,
        BoolValueTraits::kValueFieldType
    );
    return formats::json::ValueBuilder{message.GetReflection()->GetBool(message, &field_desc)};
}

formats::json::ValueBuilder WriteStringValueMessage(const ::google::protobuf::Message& message, const PrintOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        StringValueTraits::kValueFieldNumber,
        StringValueTraits::kValueFieldType
    );

    using StringType = decltype(message.GetReflection()->GetString(message, &field_desc));

    StringType scratch;
    std::string_view str = message.GetReflection()->GetStringReference(message, &field_desc, &scratch);
    return formats::json::ValueBuilder{str};
}

formats::json::ValueBuilder WriteBytesValueMessage(const ::google::protobuf::Message& message, const PrintOptions&) {
    const auto& field_desc = GetMessageFieldDesc(
        *message.GetDescriptor(),
        BytesValueTraits::kValueFieldNumber,
        BytesValueTraits::kValueFieldType
    );
    return formats::json::ValueBuilder{
        crypto::base64::Base64Encode(message.GetReflection()->GetString(message, &field_desc))
    };
}

}  // namespace

formats::json::ValueBuilder WriteMessage(const ::google::protobuf::Message& message, const PrintOptions& options) try
{
    const auto write = GetWriteMessageFunc(message.GetDescriptor()->full_name());
    return write(message, options);
} catch (FieldError& error) {
    throw PrintError(error.GetCode<PrintErrorCode>(), std::move(error).GetPath());
}

}  // namespace protobuf::json::impl

USERVER_NAMESPACE_END
