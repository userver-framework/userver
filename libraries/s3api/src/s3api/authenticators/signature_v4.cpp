#include <userver/s3api/authenticators/signature_v4.hpp>

#include <algorithm>
#include <chrono>
#include <map>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <boost/algorithm/string.hpp>

#include <userver/crypto/hash.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/s3api/authenticators/utils.hpp>
#include <userver/s3api/models/request.hpp>
#include <userver/utils/datetime_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::authenticators {

namespace {

constexpr std::string_view kAlgorithm = "AWS4-HMAC-SHA256";
constexpr std::string_view kAws4Request = "aws4_request";
constexpr std::string_view kUnsignedPayload = "UNSIGNED-PAYLOAD";

bool IsUnreservedChar(char c) {
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        return true;
    }
    return c == '-' || c == '_' || c == '.' || c == '~';
}

void PercentEncodeByteTo(unsigned char byte, std::string& result) {
    static constexpr char kHexDigits[] = "0123456789ABCDEF";
    result.push_back('%');
    result.push_back(kHexDigits[byte >> 4]);
    result.push_back(kHexDigits[byte & 0x0F]);
}

std::string UriEncode(std::string_view value, bool encode_slash) {
    std::string result;
    result.reserve(value.size());

    for (auto c : value) {
        if (IsUnreservedChar(c) || (c == '/' && !encode_slash)) {
            result.push_back(c);
        } else {
            PercentEncodeByteTo(static_cast<unsigned char>(c), result);
        }
    }

    return result;
}

std::optional<int> ParseHexDigit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return std::nullopt;
}

std::string PercentDecode(std::string_view value) {
    std::string result;
    result.reserve(value.size());

    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            const auto high = ParseHexDigit(value[i + 1]);
            const auto low = ParseHexDigit(value[i + 2]);
            if (high && low) {
                result.push_back(static_cast<char>((*high * 16) + *low));
                i += 2;
                continue;
            }
        }
        result.push_back(value[i]);
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

struct RequestTarget {
    std::string_view path;
    std::string_view query;
};

RequestTarget SplitRequestTarget(const std::string& req) {
    const std::string_view target{req};
    const auto query_pos = target.find('?');

    if (query_pos == std::string_view::npos) {
        return RequestTarget{
            .path = target,
            .query = {},
        };
    }

    return RequestTarget{
        .path = target.substr(0, query_pos),
        .query = target.substr(query_pos + 1),
    };
}

bool IsVirtualHostAddressing(std::string_view host, std::string_view bucket) {
    if (bucket.empty()) {
        return true;
    }
    if (host.size() <= bucket.size() || host[bucket.size()] != '.') {
        return false;
    }
    return host.substr(0, bucket.size()) == bucket;
}

std::string MakeCanonicalUri(const Request& request, std::string_view host, std::string_view path) {
    std::string raw_path;

    if (!IsVirtualHostAddressing(host, request.bucket)) {
        raw_path = request.bucket + "/";
    }

    raw_path += PercentDecode(path);

    return "/" + UriEncode(raw_path, /*encode_slash=*/false);
}

using QueryParams = std::vector<std::pair<std::string, std::string>>;

QueryParams ParseQuery(std::string_view query) {
    QueryParams result;

    while (!query.empty()) {
        const auto param = query.substr(0, query.find('&'));
        query.remove_prefix(std::min(query.size(), param.size() + 1));

        if (param.empty()) {
            continue;
        }

        const auto eq_pos = param.find('=');
        if (eq_pos == std::string_view::npos) {
            result.emplace_back(PercentDecode(param), std::string{});
        } else {
            result.emplace_back(PercentDecode(param.substr(0, eq_pos)), PercentDecode(param.substr(eq_pos + 1)));
        }
    }

    return result;
}

std::string MakeCanonicalQueryString(QueryParams params) {
    for (auto& [name, value] : params) {
        name = UriEncode(name, /*encode_slash=*/true);
        value = UriEncode(value, /*encode_slash=*/true);
    }
    std::ranges::sort(params);

    std::string result;

    for (const auto& [name, value] : params) {
        if (!result.empty()) {
            result.push_back('&');
        }
        result.append(name);
        result.push_back('=');
        result.append(value);
    }

    return result;
}

struct CanonicalHeaders {
    // "name1:value1\nname2:value2\n" with lowercase names sorted alphabetically
    std::string headers;
    // "name1;name2"
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

std::string MakeCanonicalRequest(
    const Request& request,
    std::string_view host,
    const CanonicalHeaders& canonical_headers,
    QueryParams extra_query_params,
    std::string_view payload_hash
) {
    const auto target = SplitRequestTarget(request.req);

    auto query_params = ParseQuery(target.query);
    std::ranges::move(extra_query_params, std::back_inserter(query_params));

    return fmt::format(
        "{}\n{}\n{}\n{}\n{}\n{}",
        ToStringView(request.method),
        MakeCanonicalUri(request, host, target.path),
        MakeCanonicalQueryString(std::move(query_params)),
        canonical_headers.headers,
        canonical_headers.signed_headers,
        payload_hash
    );
}

struct SigningScope {
    std::time_t now{};
    std::string amz_date;
    std::string date_stamp;
    std::string credential_scope;
};

SigningScope MakeSigningScope(std::string_view region, std::string_view service) {
    const auto now = utils::datetime::Now();

    SigningScope scope;
    scope.now = std::chrono::system_clock::to_time_t(now);
    scope.amz_date = utils::datetime::UtcTimestring(now, "%Y%m%dT%H%M%SZ");
    scope.date_stamp = scope.amz_date.substr(0, 8);  // 4 - year, 2 - month, 2 - day
    scope.credential_scope = fmt::format("{}/{}/{}/{}", scope.date_stamp, region, service, kAws4Request);

    return scope;
}

std::string MakeStringToSign(std::string_view canonical_request, const SigningScope& scope) {
    return fmt::format(
        "{}\n{}\n{}\n{}",
        kAlgorithm,
        scope.amz_date,
        scope.credential_scope,
        crypto::hash::Sha256(canonical_request, crypto::hash::OutputEncoding::kHex)
    );
}

std::string MakeSignature(
    std::string_view string_to_sign,
    const SigningScope& scope,
    std::string_view region,
    std::string_view service,
    const Secret& secret_key
) {
    // https://docs.aws.amazon.com/AmazonS3/latest/developerguide/sigv4-query-string-auth.html#query-string-auth-v4-signing

    static constexpr auto kBinary = crypto::hash::OutputEncoding::kBinary;

    auto key = crypto::hash::HmacSha256("AWS4" + secret_key.GetUnderlying(), scope.date_stamp, kBinary);
    key = crypto::hash::HmacSha256(key, region, kBinary);
    key = crypto::hash::HmacSha256(key, service, kBinary);
    key = crypto::hash::HmacSha256(key, kAws4Request, kBinary);

    return crypto::hash::HmacSha256(key, string_to_sign, crypto::hash::OutputEncoding::kHex);
}

std::string GetHostHeaderValue(const Request& request) {
    const auto it = request.headers.find(USERVER_NAMESPACE::http::headers::kHost);
    if (it == request.headers.end() || it->second.empty()) {
        throw std::runtime_error("AWS Signature V4 requires the 'Host' header, set it before signing the request");
    }
    return TrimAndCollapseSpaces(it->second);
}

}  // namespace

std::unordered_map<std::string, std::string> SignatureV4::Auth(const Request& request) const {
    // https://docs.aws.amazon.com/AmazonS3/latest/developerguide/sig-v4-header-based-auth.html

    const auto scope = MakeSigningScope(region_, service_);
    const auto host = GetHostHeaderValue(request);
    auto payload_hash = crypto::hash::Sha256(request.body, crypto::hash::OutputEncoding::kHex);

    std::map<std::string, std::string> headers_to_sign;
    for (const auto& [name, value] : request.headers) {
        headers_to_sign[boost::algorithm::to_lower_copy(name)] = TrimAndCollapseSpaces(value);
    }
    headers_to_sign["host"] = host;
    headers_to_sign["x-amz-date"] = scope.amz_date;
    headers_to_sign["x-amz-content-sha256"] = payload_hash;

    const auto canonical_headers = MakeCanonicalHeaders(headers_to_sign);
    const auto canonical_request = MakeCanonicalRequest(request, host, canonical_headers, {}, payload_hash);
    const auto string_to_sign = MakeStringToSign(canonical_request, scope);
    const auto signature = MakeSignature(string_to_sign, scope, region_, service_, secret_key_);

    auto authorization = fmt::format(
        "{} Credential={}/{}, SignedHeaders={}, Signature={}",
        kAlgorithm,
        access_key_,
        scope.credential_scope,
        canonical_headers.signed_headers,
        signature
    );

    return {
        {"Authorization", std::move(authorization)},
        {"X-Amz-Date", scope.amz_date},
        {"X-Amz-Content-Sha256", std::move(payload_hash)},
    };
}

std::unordered_map<std::string, std::string> SignatureV4::Sign(const Request& request, std::time_t expires) const {
    // https://docs.aws.amazon.com/AmazonS3/latest/developerguide/sigv4-query-string-auth.html

    const auto scope = MakeSigningScope(region_, service_);
    const auto host = GetHostHeaderValue(request);

    const auto expires_in = std::max<std::time_t>(expires - scope.now, 1);

    std::unordered_map<std::string, std::string> sign_params{
        {"X-Amz-Algorithm", std::string{kAlgorithm}},
        {"X-Amz-Credential", fmt::format("{}/{}", access_key_, scope.credential_scope)},
        {"X-Amz-Date", scope.amz_date},
        {"X-Amz-Expires", std::to_string(expires_in)},
        {"X-Amz-SignedHeaders", "host"},
    };

    const CanonicalHeaders canonical_headers{
        .headers = fmt::format("host:{}\n", host),
        .signed_headers = "host",
    };

    const auto canonical_request = MakeCanonicalRequest(
        request,
        host,
        canonical_headers,
        QueryParams{sign_params.begin(), sign_params.end()},
        kUnsignedPayload
    );

    const auto string_to_sign = MakeStringToSign(canonical_request, scope);

    sign_params.emplace("X-Amz-Signature", MakeSignature(string_to_sign, scope, region_, service_, secret_key_));

    return sign_params;
}

}  // namespace s3api::authenticators

USERVER_NAMESPACE_END
