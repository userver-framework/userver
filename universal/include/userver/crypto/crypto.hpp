#pragma once

/// @file userver/crypto/crypto.hpp
/// @brief Include-all header for crypto routines

#include <userver/crypto/algorithm.hpp>
#include <userver/crypto/base64.hpp>
#include <userver/crypto/certificate.hpp>
#include <userver/crypto/exception.hpp>
#include <userver/crypto/hash.hpp>
#include <userver/crypto/private_key.hpp>
#include <userver/crypto/public_key.hpp>
#include <userver/crypto/signers.hpp>
#include <userver/crypto/verifiers.hpp>

USERVER_NAMESPACE_BEGIN

/// Cryptography support
namespace crypto {}

USERVER_NAMESPACE_END
