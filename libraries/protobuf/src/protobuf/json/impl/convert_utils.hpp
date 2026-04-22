#pragma once

#include <string_view>

#include <google/protobuf/descriptor.h>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::impl {

constexpr inline std::string_view kNan = "NaN";
constexpr inline std::string_view kPositiveInf = "Infinity";
constexpr inline std::string_view kNegativeInf = "-Infinity";

// locale-independent functions
constexpr inline bool ascii_isdigit(const char c) noexcept { return c >= '0' && c <= '9'; }

constexpr inline bool ascii_islower(const char c) noexcept { return c >= 'a' && c <= 'z'; }

constexpr inline bool ascii_isupper(const char c) noexcept { return c >= 'A' && c <= 'Z'; }

constexpr inline char ascii_tolower(const char c) noexcept { return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c; }

constexpr inline char ascii_toupper(const char c) noexcept { return (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c; }

enum class MessageType {
    kGeneral = 1,
    kAny = 2,
    kDuration = 3,
    kTimestamp = 4,
    kFieldMask = 5,
    kValue = 6,
    kListValue = 7,
    kStruct = 8,
    kDoubleValue = 9,
    kFloatValue = 10,
    kInt64Value = 11,
    kUInt64Value = 12,
    kInt32Value = 13,
    kUInt32Value = 14,
    kBoolValue = 15,
    kStringValue = 16,
    kBytesValue = 17
};

[[nodiscard]] MessageType ClassifyMessage(std::string_view full_name) noexcept;

[[nodiscard]] inline bool IsNullValue(const ::google::protobuf::EnumDescriptor& desc) {
    constexpr std::string_view kNullValueName = "google.protobuf.NullValue";
    return desc.full_name() == kNullValueName;
}

[[nodiscard]] inline bool IsNullValue(const ::google::protobuf::FieldDescriptor& desc) {
    return desc.type() == ::google::protobuf::FieldDescriptor::TYPE_ENUM ? IsNullValue(*desc.enum_type()) : false;
}

[[nodiscard]] const ::google::protobuf::FieldDescriptor& GetMessageFieldDesc(
    const ::google::protobuf::Descriptor& message_desc,
    int field_number,
    ::google::protobuf::FieldDescriptor::Type field_type,
    bool is_repeated = false
);

[[nodiscard]] const ::google::protobuf::Descriptor* FindMessageDescByTypeUrl(
    const ::google::protobuf::DescriptorPool& pool,
    std::string_view type_url
) noexcept;

struct AnyTraits {
    static constexpr inline int kTypeUrlFieldNumber = 1;
    static constexpr inline ::google::protobuf::FieldDescriptor::Type
        kTypeUrlFieldType = ::google::protobuf::FieldDescriptor::TYPE_STRING;
    static constexpr inline int kValueFieldNumber = 2;
    static constexpr inline ::google::protobuf::FieldDescriptor::Type
        kValueFieldType = ::google::protobuf::FieldDescriptor::TYPE_BYTES;
};

struct DurationTraits {
    static constexpr inline int kSecondsFieldNumber = 1;
    static constexpr inline ::google::protobuf::FieldDescriptor::Type
        kSecondsFieldType = ::google::protobuf::FieldDescriptor::TYPE_INT64;
    static constexpr inline int kNanosFieldNumber = 2;
    static constexpr inline ::google::protobuf::FieldDescriptor::Type
        kNanosFieldType = ::google::protobuf::FieldDescriptor::TYPE_INT32;
};

struct TimestampTraits {
    static constexpr inline int kSecondsFieldNumber = 1;
    static constexpr inline ::google::protobuf::FieldDescriptor::Type
        kSecondsFieldType = ::google::protobuf::FieldDescriptor::TYPE_INT64;
    static constexpr inline int kNanosFieldNumber = 2;
    static constexpr inline ::google::protobuf::FieldDescriptor::Type
        kNanosFieldType = ::google::protobuf::FieldDescriptor::TYPE_INT32;
};

struct FieldMaskTraits {
    static constexpr inline int kPathsFieldNumber = 1;
    static constexpr inline ::google::protobuf::FieldDescriptor::Type
        kPathsFieldType = ::google::protobuf::FieldDescriptor::TYPE_STRING;
};

struct ValueTraits {
    static constexpr inline int kNullFieldNumber = 1;
    static constexpr inline ::google::protobuf::FieldDescriptor::Type
        kNullFieldType = ::google::protobuf::FieldDescriptor::TYPE_ENUM;
    static constexpr inline int kNumberFieldNumber = 2;
    static constexpr inline ::google::protobuf::FieldDescriptor::Type
        kNumberFieldType = ::google::protobuf::FieldDescriptor::TYPE_DOUBLE;
    static constexpr inline int kStringFieldNumber = 3;
    static constexpr inline ::google::protobuf::FieldDescriptor::Type
        kStringFieldType = ::google::protobuf::FieldDescriptor::TYPE_STRING;
    static constexpr inline int kBoolFieldNumber = 4;
    static constexpr inline ::google::protobuf::FieldDescriptor::Type
        kBoolFieldType = ::google::protobuf::FieldDescriptor::TYPE_BOOL;
    static constexpr inline int kStructFieldNumber = 5;
    static constexpr inline ::google::protobuf::FieldDescriptor::Type
        kStructFieldType = ::google::protobuf::FieldDescriptor::TYPE_MESSAGE;
    static constexpr inline int kListFieldNumber = 6;
    static constexpr inline ::google::protobuf::FieldDescriptor::Type
        kListFieldType = ::google::protobuf::FieldDescriptor::TYPE_MESSAGE;
};

struct ListValueTraits {
    static constexpr inline int kValuesFieldNumber = 1;
    static constexpr inline ::google::protobuf::FieldDescriptor::Type
        kValuesFieldType = ::google::protobuf::FieldDescriptor::TYPE_MESSAGE;
};

struct StructTraits {
    static constexpr inline int kFieldsFieldNumber = 1;
    static constexpr inline ::google::protobuf::FieldDescriptor::Type
        kFieldsFieldType = ::google::protobuf::FieldDescriptor::TYPE_MESSAGE;
};

template <::google::protobuf::FieldDescriptor::Type kType>
struct WrapperTraits {
    static constexpr inline int kValueFieldNumber = 1;
    static constexpr inline ::google::protobuf::FieldDescriptor::Type kValueFieldType = kType;
};

using DoubleValueTraits = WrapperTraits<::google::protobuf::FieldDescriptor::TYPE_DOUBLE>;
using FloatValueTraits = WrapperTraits<::google::protobuf::FieldDescriptor::TYPE_FLOAT>;
using Int64ValueTraits = WrapperTraits<::google::protobuf::FieldDescriptor::TYPE_INT64>;
using UInt64ValueTraits = WrapperTraits<::google::protobuf::FieldDescriptor::TYPE_UINT64>;
using Int32ValueTraits = WrapperTraits<::google::protobuf::FieldDescriptor::TYPE_INT32>;
using UInt32ValueTraits = WrapperTraits<::google::protobuf::FieldDescriptor::TYPE_UINT32>;
using BoolValueTraits = WrapperTraits<::google::protobuf::FieldDescriptor::TYPE_BOOL>;
using StringValueTraits = WrapperTraits<::google::protobuf::FieldDescriptor::TYPE_STRING>;
using BytesValueTraits = WrapperTraits<::google::protobuf::FieldDescriptor::TYPE_BYTES>;

}  // namespace protobuf::json::impl

USERVER_NAMESPACE_END
