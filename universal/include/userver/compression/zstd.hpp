#pragma once

/// @file userver/compression/zstd.hpp
/// @brief @copybrief compression::zstd::Decompress
/// @ingroup userver_universal

#include <string_view>

#include <userver/compression/error.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief Zstandard (de)compression helpers.
namespace compression::zstd {

extern const int kDefaultCompressionLevel;

/// @brief Decompresses all Zstandard frames in the input, ignoring skippable frames.
/// @throws std::runtime_error if the input is invalid or truncated.
/// @throws compression::TooBigError if the combined output exceeds `max_size`.
std::string Decompress(std::string_view compressed, size_t max_size);

/// @brief Compresses the string using the specified compression level.
std::string Compress(std::string_view data, int compression_level = kDefaultCompressionLevel);

}  // namespace compression::zstd

USERVER_NAMESPACE_END
