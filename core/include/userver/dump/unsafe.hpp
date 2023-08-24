#pragma once

#include <string_view>

#include <userver/dump/operations.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

/// @brief Writes a non-size-prefixed `std::string_view`
/// @note `writer.Write(str)` should normally be used instead to write strings
void WriteStringViewUnsafe(Writer& writer, std::string_view value);

/// @brief Reads a `std::string_view`
/// @warning The `string_view` will be invalidated on the next `Read` operation
std::string_view ReadStringViewUnsafe(Reader& reader);

/// @brief Reads a non-size-prefixed `std::string_view`
/// @note The caller must somehow know the string size in advance
/// @warning The `string_view` will be invalidated on the next `Read` operation
std::string_view ReadStringViewUnsafe(Reader& reader, std::size_t size);

/// @brief Reads a `std::string_view`
/// @note Normally, exactly `max_size` bytes is returned. On end-of-file,
/// the amount of bytes returned can be less than `max_size`.
/// @warning The `string_view` will be invalidated on the next `Read` operation
std::string_view ReadUnsafeAtMost(Reader& reader, std::size_t max_size);

}  // namespace dump

USERVER_NAMESPACE_END
