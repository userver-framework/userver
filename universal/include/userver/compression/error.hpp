#pragma once

/// @file userver/compression/error.hpp
/// @brief Decompression errors
/// @ingroup userver_universal

#include <stdexcept>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

/// @brief Compression and decompression utilities and errors.
namespace compression {

/// Base class for decompression errors
class DecompressionError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;

    DecompressionError(const DecompressionError&) = default;
    DecompressionError(DecompressionError&&) = default;
    DecompressionError& operator=(const DecompressionError&) = default;
    DecompressionError& operator=(DecompressionError&&) = default;

    ~DecompressionError() override;
};

/// Decompressed data size exceeds the limit
class TooBigError : public DecompressionError {
public:
    TooBigError()
        : DecompressionError("Decompressed data exceeds the limit")
    {}

    TooBigError(const TooBigError&) = default;
    TooBigError(TooBigError&&) = default;
    TooBigError& operator=(const TooBigError&) = default;
    TooBigError& operator=(TooBigError&&) = default;

    ~TooBigError() override;
};

class ErrWithCode : public DecompressionError {
public:
    explicit ErrWithCode(const char* err_name)
        : DecompressionError(fmt::format("Decompression failed: {}", err_name))
    {}

    ErrWithCode(const ErrWithCode&) = default;
    ErrWithCode(ErrWithCode&&) = default;
    ErrWithCode& operator=(const ErrWithCode&) = default;
    ErrWithCode& operator=(ErrWithCode&&) = default;

    ~ErrWithCode() override;
};

}  // namespace compression

USERVER_NAMESPACE_END
