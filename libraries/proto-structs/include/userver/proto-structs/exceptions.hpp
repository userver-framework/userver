#pragma once

/// @file userver/proto-structs/exceptions.hpp
/// @brief Exceptions thrown by the library

#include <stdexcept>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

/// @brief Library basic exception type
/// All other proto-structs exceptions are derived from this type.
class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// @brief Conversion error
/// Exception is thrown if protobuf message field can't be converted to/from struct field.
class ConversionError : public Error {
public:
    /// @brief Creates error signalling that conversion failed for field @a field_name of message @a message_name
    ConversionError(std::string_view message_name, std::string_view field_name, std::string_view reason);
};

/// @brief Trying to access unset @ref proto_structs::Oneof field
/// @note This exception is also thrown if @ref proto_structs::Oneof is cleared
class OneofAccessError : public Error {
public:
    /// @brief Creates error when attempting to access @a field_idx field of `oneof`
    explicit OneofAccessError(std::size_t field_idx);
};

/// @brief Failed to pack struct's compatible message to @ref proto_structs::Any underlying storage.
/// @note This exception is thrown *after* struct to protobuf message conversion.
class AnyPackError : public Error {
public:
    /// @brief Creates error when trying to pack protobuf message @a message_name
    explicit AnyPackError(std::string_view message_name);
};

/// @brief Failed to unpack struct's compatible message from @ref proto_structs::Any underlying storage
/// @note This exception is thrown *before* protobuf message to struct conversion.
class AnyUnpackError : public Error {
public:
    /// @brief Creates error when trying to unpack protobuf message @a message_name
    explicit AnyUnpackError(std::string_view message_name);
};

}  // namespace proto_structs

USERVER_NAMESPACE_END
