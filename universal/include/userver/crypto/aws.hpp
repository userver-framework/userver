#pragma once

/// @file userver/crypto/aws.hpp
/// @brief AWS Signature Version 4 helpers
/// @ingroup userver_universal

#include <ctime>
#include <string>
#include <string_view>

#include <userver/http/header_map.hpp>
#include <userver/http/predefined_header.hpp>

USERVER_NAMESPACE_BEGIN

/// AWS Signature Version 4
namespace crypto::aws {

/// @see https://docs.aws.amazon.com/IAM/latest/UserGuide/reference_sigv-create-signed-request.html
inline constexpr std::string_view kAws4HmacSha256 = "AWS4-HMAC-SHA256";
inline constexpr std::string_view kAws4Request = "aws4_request";
inline constexpr std::string_view kUnsignedPayload = "UNSIGNED-PAYLOAD";

inline constexpr USERVER_NAMESPACE::http::headers::PredefinedHeader kAmzDate{"X-Amz-Date"};
inline constexpr USERVER_NAMESPACE::http::headers::PredefinedHeader kAmzContentSha256{"X-Amz-Content-Sha256"};

/// Inputs for header-based AWS Signature Version 4.
///
/// `Host` must already be present in the header map. `canonical_uri` and
/// `canonical_query` are service-specific (see the SigV4 docs); they are not
/// derived from the headers.
struct SignV4Request {
    std::string_view http_method;
    std::string_view canonical_uri;
    std::string_view canonical_query{};
    std::string_view payload{};
    std::string_view access_key;
    std::string_view secret_key;
    std::string_view region;
    std::string_view service;
};

/// Date and credential scope used to sign a request.
struct V4TimeScope {
    std::time_t now{};
    std::string amz_date;
    std::string date_stamp;
    std::string credential_scope;
};

/// Adds `X-Amz-Date`, `X-Amz-Content-Sha256` and `Authorization` to @a headers.
///
/// @throws std::runtime_error if `Host` is missing or empty
void SignRequestV4(USERVER_NAMESPACE::http::headers::HeaderMap& headers, const SignV4Request& request);

/// Builds `YYYYMMDDThhmmssZ`, `YYYYMMDD` and `{date}/{region}/{service}/aws4_request`
/// from the current time (`utils::datetime::Now`, mockable in tests).
V4TimeScope MakeV4TimeScope(std::string_view region, std::string_view service);

/// Canonical request string: method, URI, query, headers, signed headers, payload hash.
std::string MakeCanonicalRequest(
    std::string_view http_method,
    std::string_view canonical_uri,
    std::string_view canonical_query,
    std::string_view canonical_headers,
    std::string_view signed_headers,
    std::string_view payload_hash
);

/// `AWS4-HMAC-SHA256` string to sign for @a canonical_request.
std::string MakeV4StringToSign(std::string_view canonical_request, const V4TimeScope& scope);

/// HMAC-SHA256 signature of @a string_to_sign with the derived signing key.
std::string MakeV4Signature(
    std::string_view string_to_sign,
    const V4TimeScope& scope,
    std::string_view region,
    std::string_view service,
    std::string_view secret_key
);

}  // namespace crypto::aws

USERVER_NAMESPACE_END
