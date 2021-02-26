#pragma once

#include <string_view>

#include <cache/dump/operations.hpp>

namespace cache::dump {

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

}  // namespace cache::dump
