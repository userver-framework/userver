#include <userver/s3api/authenticators/utils.hpp>

#include <map>
#include <optional>
#include <set>
#include <sstream>

#include <boost/algorithm/string.hpp>

#include <userver/clients/http/request.hpp>
#include <userver/crypto/hash.hpp>
#include <userver/utils/datetime_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::authenticators {

std::string HttpMethodToString(const clients::http::HttpMethod http_method) {
    std::string http_method_string;

    switch (http_method) {
        case clients::http::HttpMethod::kDelete:
            http_method_string = "DELETE";
            break;
        case clients::http::HttpMethod::kGet:
            http_method_string = "GET";
            break;
        case clients::http::HttpMethod::kHead:
            http_method_string = "HEAD";
            break;
        case clients::http::HttpMethod::kPost:
            http_method_string = "POST";
            break;
        case clients::http::HttpMethod::kPut:
            http_method_string = "PUT";
            break;
        case clients::http::HttpMethod::kPatch:
            http_method_string = "PATCH";
            break;
        default:
            throw std::runtime_error("Unknown http method");
    }

    return http_method_string;
}

std::string RemoveExcessiveSpaces(std::string value) {
    std::string result;
    std::replace(value.begin(), value.end(), '\n', ' ');
    unique_copy(value.begin(), value.end(), back_inserter(result), [](char a, char b) { return a == ' ' && b == ' '; });
    return result;
}

std::string MakeHeaderDate() { return utils::datetime::UtcTimestring(utils::datetime::Now(), "%a, %d %b %Y %T %z"); }

std::string MakeHeaderContentMd5(const std::string& data) {
    return crypto::hash::weak::Md5(data, crypto::hash::OutputEncoding::kBase64);
}

std::string MakeStringToSign(
    const Request& request,
    const std::string& header_date,
    const std::optional<std::string>& header_content_md5
) {
    std::ostringstream signature;

    signature << HttpMethodToString(request.method) << '\n';

    // md5
    {
        if (header_content_md5) {
            signature << *header_content_md5;
        }

        signature << '\n';
    }

    // content type
    {
        static const std::string kContentType{"Content-Type"};

        const auto it = request.headers.find(kContentType);

        if (it != request.headers.cend()) {
            signature << it->second;
        }

        signature << '\n';
    }

    // date
    signature << header_date << '\n';

    // CanonicalizedAmzHeaders
    {
        std::vector<std::pair<std::string, std::string>> canonical_headers;
        canonical_headers.reserve(request.headers.size());

        std::copy_if(
            request.headers.begin(),
            request.headers.end(),
            std::back_inserter(canonical_headers),
            [](const auto& header) {
                static const std::string kAmzHeader = "x-amz-";
                auto header_start = header.first.substr(0, kAmzHeader.size());
                boost::to_lower(header_start);
                return header_start == kAmzHeader;
            }
        );
        std::for_each(canonical_headers.begin(), canonical_headers.end(), [](auto& header) {
            boost::to_lower(header.first);
        });
        std::sort(canonical_headers.begin(), canonical_headers.end(), [](const auto& header1, const auto& header2) {
            return header1.first < header2.first;
        });

        for (const auto& [header, value] : canonical_headers) {
            signature << header << ':' << RemoveExcessiveSpaces(value) << '\n';
        }
    }

    // CanonicalizedResource
    {
        // bucket
        if (!request.bucket.empty()) {
            signature << '/' + request.bucket;
        }

        auto actual_subresources = std::set<std::string>{
            "acl",
            "lifecycle",
            "location",
            "logging",
            "notification",
            "partNumber",
            "policy",
            "requestPayment",
            "uploadId",
            "uploads",
            "versionId",
            "versioning",
            "versions",
            "website"};

        // query
        { signature << '/' + request.req.substr(0, request.req.find('?')); }

        if (auto pos = request.req.find('?'); pos != std::string::npos) {
            std::vector<std::string> subresources_strings;
            auto query = request.req.substr(pos + 1);
            boost::split(subresources_strings, query, [](char c) { return c == '&'; });

            std::map<std::string, std::optional<std::string>> subresources;
            for (auto&& subresource : subresources_strings) {
                std::optional<std::string> parameter_value = std::nullopt;

                if (auto eq_pos = subresource.find('='); eq_pos != std::string::npos) {
                    parameter_value.emplace(subresource.substr(eq_pos + 1));
                    subresource.resize(eq_pos);
                }

                if (actual_subresources.count(subresource) != 0) {
                    subresources.emplace(std::move(subresource), std::move(parameter_value));
                }
            }

            bool is_first = true;
            for (auto& [subresource, value] : subresources) {
                signature << (is_first ? "?" : "&");
                is_first = false;
                if (value) {
                    signature << fmt::format("{}={}", subresource, *value);
                } else {
                    signature << subresource;
                }
            }
        }
    }

    return signature.str();
}

std::string MakeSignature(const std::string& string_to_sign, const Secret& secret_key) {
    return crypto::hash::HmacSha1(secret_key.GetUnderlying(), string_to_sign, crypto::hash::OutputEncoding::kBase64);
}

std::string
MakeHeaderAuthorization(const std::string& string_to_sign, const std::string& access_key, const Secret& secret_key) {
    return "AWS " + access_key + ":" + MakeSignature(string_to_sign, secret_key);
}

}  // namespace s3api::authenticators

USERVER_NAMESPACE_END
