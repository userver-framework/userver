#pragma once

/// @file userver/s3api/authenticators/utils.hpp
/// @brief Helpers for writing your own authenticators

#include <string>

#include <userver/s3api/models/request.hpp>
#include <userver/s3api/models/secret.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::authenticators {

std::string MakeHeaderDate();
std::string MakeHeaderContentMd5(const std::string& data);
std::string
MakeHeaderAuthorization(const std::string& string_to_sign, const std::string& access_key, const Secret& secret_key);
std::string MakeSignature(const std::string& string_to_sign, const Secret& secret_key);

std::string MakeStringToSign(
    const Request& request,
    const std::string& header_date,
    const std::optional<std::string>& header_content_md5
);

}  // namespace s3api::authenticators

USERVER_NAMESPACE_END
