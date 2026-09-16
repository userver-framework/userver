#include <userver/s3api/authenticators/signature_v4.hpp>

#include <algorithm>
#include <cctype>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <userver/clients/http/request.hpp>
#include <userver/crypto/aws.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/s3api/models/request.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::authenticators {

namespace {

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

std::string UriDecode(std::string_view value) {
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

    raw_path += UriDecode(path);

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
            result.emplace_back(UriDecode(param), std::string{});
        } else {
            result.emplace_back(UriDecode(param.substr(0, eq_pos)), UriDecode(param.substr(eq_pos + 1)));
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

    const auto host = GetHostHeaderValue(request);
    const auto target = SplitRequestTarget(request.req);
    const auto canonical_uri = MakeCanonicalUri(request, host, target.path);
    const auto canonical_query = MakeCanonicalQueryString(ParseQuery(target.query));

    auto headers = request.headers;
    crypto::aws::SignRequestV4(
        headers,
        {
            .http_method = ToStringView(request.method),
            .canonical_uri = canonical_uri,
            .canonical_query = canonical_query,
            .payload = request.body,
            .access_key = access_key_,
            .secret_key = secret_key_.GetUnderlying(),
            .region = region_,
            .service = service_,
        }
    );

    return {
        {"Authorization", headers[USERVER_NAMESPACE::http::headers::kAuthorization]},
        {"X-Amz-Date", headers[crypto::aws::kAmzDate]},
        {"X-Amz-Content-Sha256", headers[crypto::aws::kAmzContentSha256]},
    };
}

std::unordered_map<std::string, std::string> SignatureV4::Sign(const Request& request, std::time_t expires) const {
    // https://docs.aws.amazon.com/AmazonS3/latest/developerguide/sigv4-query-string-auth.html

    const auto scope = crypto::aws::MakeV4TimeScope(region_, service_);
    const auto host = GetHostHeaderValue(request);
    const auto target = SplitRequestTarget(request.req);

    const auto expires_in = std::max<std::time_t>(expires - scope.now, 1);

    std::unordered_map<std::string, std::string> sign_params{
        {"X-Amz-Algorithm", std::string{crypto::aws::kAws4HmacSha256}},
        {"X-Amz-Credential", fmt::format("{}/{}", access_key_, scope.credential_scope)},
        {"X-Amz-Date", scope.amz_date},
        {"X-Amz-Expires", std::to_string(expires_in)},
        {"X-Amz-SignedHeaders", "host"},
    };

    QueryParams query_params = ParseQuery(target.query);
    query_params.insert(query_params.end(), sign_params.begin(), sign_params.end());

    const auto canonical_request = crypto::aws::MakeCanonicalRequest(
        ToStringView(request.method),
        MakeCanonicalUri(request, host, target.path),
        MakeCanonicalQueryString(std::move(query_params)),
        fmt::format("host:{}\n", host),
        "host",
        crypto::aws::kUnsignedPayload
    );

    const auto string_to_sign = crypto::aws::MakeV4StringToSign(canonical_request, scope);
    sign_params.emplace(
        "X-Amz-Signature",
        crypto::aws::MakeV4Signature(string_to_sign, scope, region_, service_, secret_key_.GetUnderlying())
    );

    return sign_params;
}

}  // namespace s3api::authenticators

USERVER_NAMESPACE_END
