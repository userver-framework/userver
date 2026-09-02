#include <userver/crypto/aws.hpp>

#include <cctype>
#include <chrono>
#include <map>
#include <stdexcept>

#include <fmt/format.h>

#include <userver/crypto/hash.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/utils/datetime_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto::aws {
namespace {

std::string ToLowerAscii(std::string_view value) {
    std::string result{value};
    for (auto& c : result) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return result;
}

std::string TrimAndCollapseSpaces(std::string_view value) {
    std::string result;
    result.reserve(value.size());

    bool pending_space = false;
    for (auto c : value) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            pending_space = !result.empty();
            continue;
        }

        if (pending_space) {
            result.push_back(' ');
            pending_space = false;
        }

        result.push_back(c);
    }

    return result;
}

struct CanonicalHeaders {
    std::string headers;
    std::string signed_headers;
};

CanonicalHeaders MakeCanonicalHeaders(const std::map<std::string, std::string>& headers) {
    CanonicalHeaders result;

    for (const auto& [name, value] : headers) {
        result.headers += fmt::format("{}:{}\n", name, value);
        if (!result.signed_headers.empty()) {
            result.signed_headers.push_back(';');
        }
        result.signed_headers += name;
    }

    return result;
}

std::string GetHostHeaderValue(const USERVER_NAMESPACE::http::headers::HeaderMap& headers) {
    const auto it = headers.find(USERVER_NAMESPACE::http::headers::kHost);
    if (it == headers.end() || it->second.empty()) {
        throw std::runtime_error("AWS Signature V4 requires the 'Host' header, set it before signing the request");
    }
    return TrimAndCollapseSpaces(it->second);
}

}  // namespace

V4TimeScope MakeV4TimeScope(std::string_view region, std::string_view service) {
    const auto now = utils::datetime::Now();

    V4TimeScope scope;
    scope.now = std::chrono::system_clock::to_time_t(now);
    scope.amz_date = utils::datetime::UtcTimestring(now, "%Y%m%dT%H%M%SZ");
    scope.date_stamp = scope.amz_date.substr(0, 8);  // 4 - year, 2 - month, 2 - day
    scope.credential_scope = fmt::format("{}/{}/{}/{}", scope.date_stamp, region, service, kAws4Request);

    return scope;
}

std::string MakeCanonicalRequest(
    std::string_view http_method,
    std::string_view canonical_uri,
    std::string_view canonical_query,
    std::string_view canonical_headers,
    std::string_view signed_headers,
    std::string_view payload_hash
) {
    return fmt::format(
        "{}\n{}\n{}\n{}\n{}\n{}",
        http_method,
        canonical_uri,
        canonical_query,
        canonical_headers,
        signed_headers,
        payload_hash
    );
}

std::string MakeV4StringToSign(std::string_view canonical_request, const V4TimeScope& scope) {
    return fmt::format(
        "{}\n{}\n{}\n{}",
        kAws4HmacSha256,
        scope.amz_date,
        scope.credential_scope,
        hash::Sha256(canonical_request, hash::OutputEncoding::kHex)
    );
}

std::string MakeV4Signature(
    std::string_view string_to_sign,
    const V4TimeScope& scope,
    std::string_view region,
    std::string_view service,
    std::string_view secret_key
) {
    static constexpr auto kBinary = hash::OutputEncoding::kBinary;

    auto key = hash::HmacSha256(fmt::format("AWS4{}", secret_key), scope.date_stamp, kBinary);
    key = hash::HmacSha256(key, region, kBinary);
    key = hash::HmacSha256(key, service, kBinary);
    key = hash::HmacSha256(key, kAws4Request, kBinary);

    return hash::HmacSha256(key, string_to_sign, hash::OutputEncoding::kHex);
}

void SignRequestV4(USERVER_NAMESPACE::http::headers::HeaderMap& headers, const SignV4Request& request) {
    const auto host = GetHostHeaderValue(headers);
    const auto scope = MakeV4TimeScope(request.region, request.service);
    const auto payload_hash = hash::Sha256(request.payload, hash::OutputEncoding::kHex);

    std::map<std::string, std::string> headers_to_sign;
    for (const auto& [name, value] : headers) {
        headers_to_sign[ToLowerAscii(name)] = TrimAndCollapseSpaces(value);
    }
    headers_to_sign["host"] = host;
    headers_to_sign["x-amz-date"] = scope.amz_date;
    headers_to_sign["x-amz-content-sha256"] = payload_hash;

    const auto canonical_headers = MakeCanonicalHeaders(headers_to_sign);
    const auto canonical_request = MakeCanonicalRequest(
        request.http_method,
        request.canonical_uri,
        request.canonical_query,
        canonical_headers.headers,
        canonical_headers.signed_headers,
        payload_hash
    );
    const auto string_to_sign = MakeV4StringToSign(canonical_request, scope);
    const auto signature = MakeV4Signature(string_to_sign, scope, request.region, request.service, request.secret_key);

    headers.insert_or_assign(kAmzDate, scope.amz_date);
    headers.insert_or_assign(kAmzContentSha256, payload_hash);
    headers.insert_or_assign(
        USERVER_NAMESPACE::http::headers::kAuthorization,
        fmt::format(
            "{} Credential={}/{}, SignedHeaders={}, Signature={}",
            kAws4HmacSha256,
            request.access_key,
            scope.credential_scope,
            canonical_headers.signed_headers,
            signature
        )
    );
}

}  // namespace crypto::aws

USERVER_NAMESPACE_END
