#pragma once

#include <stdexcept>

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

}  // namespace compression
