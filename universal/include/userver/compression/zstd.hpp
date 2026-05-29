#pragma once

/// @file userver/compression/zstd.hpp
/// @brief @copybrief compression::zstd::Decompress
/// @ingroup userver_universal

#include <string_view>

#include <userver/compression/error.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief Zstandard (de)compression helpers.
namespace compression::zstd {

/// @brief Decompresses the string.
/// @throws DecompressionError
std::string Decompress(std::string_view compressed, size_t max_size);

}  // namespace compression::zstd

USERVER_NAMESPACE_END
