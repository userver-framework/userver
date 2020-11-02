#pragma once

#include <string_view>

#include <compression/error.hpp>

namespace compression::gzip {

/// Decompresses the string.
/// @throws DecompressionError
std::string Decompress(std::string_view compressed, size_t max_size);

}  // namespace compression::gzip
