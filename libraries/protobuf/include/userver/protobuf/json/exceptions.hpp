#pragma once

/// @file userver/protobuf/error.hpp
/// @brief Exceptions thrown by the JSON utilities.

#include <string>
#include <type_traits>

#include <userver/protobuf/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief Top namespace for the protobuf JSON utilities.
namespace protobuf::json {

/// @brief JSON read error code.
enum class ReadErrorCode {
    /// @brief JSON field is unknown (does not match to any protobuf message field).
    kUnknownField = 1,

    /// @brief Enum value name used as a JSON field value is unknown.
    kUnknownEnum = 2,

    /// @brief JSON contains more than one field for the same oneof in the protobuf message.
    kMultipleOneofFields = 3,

    /// @brief JSON field type is not compatible with corresponding protobuf message field type.
    kInvalidType = 4,

    /// @brief JSON field value is invalid according to ProtoJSON rules.
    kInvalidValue = 5
};

/// @brief JSON write error code.
enum class WriteErrorCode {
    /// @brief Protobuf well-known message does not have expected field
    /// @brief Protobuf message field has invalid value.
    /// This code can be set when converting well-known message which may have more strict constraints than the
    /// typesof their fields (see
    /// [here](https://github.com/protocolbuffers/protobuf/blob/main/src/google/protobuf/duration.proto) for example).
    kInvalidValue = 1
};

/// @brief JSON/protobuf conversion error information.
/// @tparam TErrorCode error code type
template <
    typename TErrorCode,
    typename = std::enable_if_t<
        std::is_same_v<TErrorCode, ReadErrorCode> || std::is_same_v<TErrorCode, WriteErrorCode>>>
class ConversionErrorInfo {
public:
    using ErrorCodeType = TErrorCode;

    /// @brief Creates error information for invalid JSON/protobuf field identified by @a path .
    /// See @ref ConversionErrorInfo::GetPath for more information about @a path format.
    ConversionErrorInfo(const ErrorCodeType code, std::string path) noexcept : code_(code), path_(std::move(path)) {}

    /// @brief Returns error code.
    [[nodiscard]] ErrorCodeType GetCode() const noexcept { return code_; }

    /// @brief Returns invalid field path.
    /// Parameter @a path format depends on the conversion direction:
    /// 1. In case of JSON to protobuf conversion, @a path will be formatted in a way used for exceptions thrown by
    ///    by various `ValueBuilder` and `Value` class methods (see@ref formats::json::ExceptionWithPath): `/` (root),
    ///    `field.array[0].item`, etc. Field names will be taken from JSON and **may not match** target field names in
    ///    the protobuf message (ProtoJSON by default uses `lowerCamelCase`-encoded protobuf field names as JSON field
    ///    names).
    /// 2. In case of protobuf message to JSON conversion, @a path will be formatted in a similar way as above, however
    ///    `map` types will be handled explicitly. Examples: `/` (root), `field.repeated[0].item.map['key'].value`, etc.
    ///    Field names will be taken from the protobuf message and **may not match** target field names in the JSON.
    [[nodiscard]] const std::string& GetPath() const& noexcept { return path_; }

    /// @brief Returns invalid field path.
    /// See @ref ConversionErrorInfo::GetPath for more information about @a path format.
    [[nodiscard]] std::string GetPath() && noexcept { return std::move(path_); }

private:
    ErrorCodeType code_;
    std::string path_;
};

/// @brief JSON read error information.
using ReadErrorInfo = ConversionErrorInfo<ReadErrorCode>;

/// @brief JSON write error information.
using WriteErrorInfo = ConversionErrorInfo<WriteErrorCode>;

/// @brief Base exception type for JSON utilities.
class JsonError : public protobuf::Error {
public:
    using protobuf::Error::Error;
};

/// @brief Base exception type for JSON/protobuf conversion errors.
class ConversionErrorBase : public protobuf::Error {
public:
    using protobuf::Error::Error;
};

/// @brief JSON/protobuf conversion error.
/// @tparam TErrorCode error code type
template <typename TErrorCode>
class ConversionError : public ConversionErrorBase {
public:
    using ErrorInfoType = ConversionErrorInfo<TErrorCode>;
    using ErrorCodeType = typename ErrorInfoType::ErrorCodeType;

    /// @brief Creates error for invalid json/protobuf field identified by @a path .
    /// See @ref ConversionErrorInfo::ConversionErrorInfo for more information about @a path format.
    ConversionError(ErrorCodeType code, std::string path);

    /// @brief Returns information about occurred error.
    [[nodiscard]] const ErrorInfoType& GetErrorInfo() const& noexcept { return error_info_; }

    /// @brief Returns information about occurred error.
    [[nodiscard]] ErrorInfoType GetErrorInfo() && noexcept { return std::move(error_info_); }

private:
    ErrorInfoType error_info_;
};

extern template class ConversionError<ReadErrorCode>;
extern template class ConversionError<WriteErrorCode>;

/// @brief JSON read error.
using ReadError = ConversionError<ReadErrorCode>;

/// @brief JSON write error.
using WriteError = ConversionError<WriteErrorCode>;

}  // namespace protobuf::json

USERVER_NAMESPACE_END
