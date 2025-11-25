#pragma once

/// @file userver/proto-structs/exceptions.hpp
/// @brief Exceptions thrown by the library.

#include <stdexcept>
#include <string_view>

USERVER_NAMESPACE_BEGIN

/// @brief Top namespace for the proto-structs library.
namespace proto_structs {

/// @brief Library basic exception type.
/// All other proto-structs exceptions are derived from this type.
class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// @brief Conversion error base class.
class ConversionError : public Error {
protected:
    using Error::Error;
};

/// @brief Error reading proto struct from protobuf message.
class ReadError final : public ConversionError {
public:
    /// @brief Creates error with information what protobuf message field was considered invalid.
    ///
    /// Parameter @a path contains dot-separated field names from the top-level message up to erroneous field, for
    /// example, "msg.field.nested_field".
    ReadError(std::string_view path, std::string_view reason);
};

/// @brief Error writing proto struct to protobuf message.
class WriteError final : public ConversionError {
public:
    /// @brief Creates error with information what protobuf message field was not correctly initialized.
    ///
    /// Parameter @a path contains dot-separated field names from the top-level message up to field that was not
    /// initialized, for example, "msg.field.nested_field".
    WriteError(std::string_view path, std::string_view reason);
};

/// @brief Incorrect value error.
class ValueError : public Error {
public:
    using Error::Error;
};

/// @brief Invalid attempt to access unset @ref proto_structs::Oneof field.
class OneofAccessError : public Error {
public:
    /// @brief Creates error on invalid attempt to access @a field_idx field of `oneof`.
    explicit OneofAccessError(std::size_t field_idx);
};

/// @brief Error packing protobuf message to @ref proto_structs::Any underlying storage.
class AnyPackError : public Error {
public:
    /// @brief Creates error with information what protobuf message was not packed.
    explicit AnyPackError(std::string_view message_name);
};

/// @brief Error unpacking protobuf message from @ref proto_structs::Any underlying storage.
///
/// The main reason of this exception is the attempt to unpack message type different from the one stored in the
/// @ref proto_structs::Any underlying storage.
class AnyUnpackError : public Error {
public:
    /// @brief Creates error with information what protobuf message was not unpacked.
    explicit AnyUnpackError(std::string_view message_name);
};

}  // namespace proto_structs

USERVER_NAMESPACE_END
