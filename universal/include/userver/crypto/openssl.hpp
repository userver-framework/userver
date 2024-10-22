#pragma once

/// @file userver/crypto/openssl.hpp
/// @brief @copybrief crypto::Openssl

USERVER_NAMESPACE_BEGIN

namespace crypto {

/// @brief Class to initialize OpenSSL with mutexes.
class Openssl final {
public:
    /// Initialize OpenSSL mutexes. Safe to call multiple times. Should be called
    /// in drivers that use low level libraries that use OpenSSL.
    static void Init() noexcept;

private:
    Openssl() noexcept;
};

}  // namespace crypto

USERVER_NAMESPACE_END
