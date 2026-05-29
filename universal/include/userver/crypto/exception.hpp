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

    CryptoException(CryptoException&&) = default;

    ~CryptoException() override;
};

/// Signature generation error
class SignError : public CryptoException {
public:
    using CryptoException::CryptoException;

    SignError(SignError&&) = default;

    ~SignError() override;
};

/// Signature verification error
class VerificationError : public CryptoException {
public:
    using CryptoException::CryptoException;

    VerificationError(VerificationError&&) = default;

    ~VerificationError() override;
};

/// Signing key parse error
class KeyParseError : public CryptoException {
public:
    using CryptoException::CryptoException;

    KeyParseError(KeyParseError&&) = default;

    ~KeyParseError() override;
};

/// Serialization error
class SerializationError : public CryptoException {
public:
    using CryptoException::CryptoException;

    SerializationError(SerializationError&&) = default;

    ~SerializationError() override;
};

}  // namespace crypto

USERVER_NAMESPACE_END
