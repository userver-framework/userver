#pragma once

/// @file userver/s3api/models/secret.hpp
/// @brief Non-loggable typedef that is used for storing secrets

#include <string>

#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api {

using Secret = USERVER_NAMESPACE::utils::NonLoggable<class SecretTag, std::string>;

}

USERVER_NAMESPACE_END
