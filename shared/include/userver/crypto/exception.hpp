#pragma once

/// @file userver/crypto/exception.hpp
/// @brief Exception classes for crypto module

#include <memory>

#include <userver/utils/traceful_exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {

/// Base exception
class CryptoException : public utils::TracefulException {
 public:
  using utils::TracefulException::TracefulException;
};

/// Signature generation error
class SignError : public CryptoException {
 public:
  using CryptoException::CryptoException;
};

/// Signature verification error
class VerificationError : public CryptoException {
 public:
  using CryptoException::CryptoException;
};

/// Signing key parse error
class KeyParseError : public CryptoException {
 public:
  using CryptoException::CryptoException;
};

}  // namespace crypto

USERVER_NAMESPACE_END
