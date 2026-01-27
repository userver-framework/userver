#include <userver/proto-structs/exceptions.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

ReadError::ReadError(const std::string_view path, const std::string_view reason)
    : ConversionError(fmt::format("Failed to read protobuf message field '{}' ({})", path, reason))
{}

WriteError::WriteError(const std::string_view path, const std::string_view reason)
    : ConversionError(fmt::format("Failed to write protobuf message field '{}' ({})", path, reason))
{}

OneofAccessError::OneofAccessError(const std::size_t field_idx)
    : Error(fmt::format("Oneof field is not set (index = {})", field_idx))
{}

AnyPackError::AnyPackError(const std::string_view message_name)
    : Error(fmt::format("Failed to pack message '{}' to 'google.protobuf.Any'", message_name))
{}

AnyUnpackError::AnyUnpackError(const std::string_view message_name)
    : Error(fmt::format("Failed to unpack message '{}' from 'google.protobuf.Any'", message_name))
{}

}  // namespace proto_structs

USERVER_NAMESPACE_END
