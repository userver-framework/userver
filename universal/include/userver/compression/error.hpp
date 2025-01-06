#pragma once

#include <stdexcept>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace compression {

/// Base class for decompression errors
class DecompressionError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

/// Decompressed data size exceeds the limit
class TooBigError : public DecompressionError {
public:
    TooBigError() : DecompressionError("Decompressed data exceeds the limit") {}
};

class ErrWithCode : public DecompressionError {
public:
    explicit ErrWithCode(const char* errName) : DecompressionError(fmt::format("Decompression failed: {}", errName)) {}
};

}  // namespace compression

USERVER_NAMESPACE_END
