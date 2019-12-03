#pragma once

#include <memory>

#include <utils/traceful_exception.hpp>

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
