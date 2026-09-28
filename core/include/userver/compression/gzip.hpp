#pragma once

#include <string_view>

#include <userver/compression/error.hpp>

USERVER_NAMESPACE_BEGIN

namespace compression::gzip {

extern const int kDefaultCompressionLevel;

/// Decompresses the string.
/// @throws DecompressionError
std::string Decompress(std::string_view compressed, size_t max_size);

/// @brief Compresses the string using the specified compression level.
std::string Compress(std::string_view data, int compression_level = kDefaultCompressionLevel);

}  // namespace compression::gzip

USERVER_NAMESPACE_END
