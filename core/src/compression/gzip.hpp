#pragma once

#include <string_view>

#include <compression/error.hpp>

USERVER_NAMESPACE_BEGIN

namespace compression::gzip {

/// Decompresses the string.
/// @throws DecompressionError
std::string Decompress(std::string_view compressed, size_t max_size);

}  // namespace compression::gzip

USERVER_NAMESPACE_END
