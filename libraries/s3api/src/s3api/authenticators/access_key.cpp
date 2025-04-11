#include <userver/s3api/authenticators/access_key.hpp>

#include <userver/s3api/authenticators/utils.hpp>

#include <optional>
#include <unordered_map>

USERVER_NAMESPACE_BEGIN

namespace s3api::authenticators {

std::unordered_map<std::string, std::string> AccessKey::Auth(const Request& request) const {
    // https://docs.aws.amazon.com/AmazonS3/latest/userguide/RESTAuthentication.html
    // without url encoding

    static const std::string kDate{"Date"};
    static const std::string kContentMd5{"Content-MD5"};
    static const std::string kAuthorization{"Authorization"};

    std::optional<std::string> header_content_md5;

    if (!request.body.empty()) {
        header_content_md5 = MakeHeaderContentMd5(request.body);
    }

    auto header_date = MakeHeaderDate();
    auto string_to_sign = MakeStringToSign(request, header_date, header_content_md5);

    std::unordered_map<std::string, std::string> auth_headers{
        {kDate, std::move(header_date)},
        {kAuthorization, MakeHeaderAuthorization(string_to_sign, access_key_, secret_key_)}};

    if (header_content_md5) {
        auth_headers.emplace(kContentMd5, std::move(*header_content_md5));
    }

    return auth_headers;
}

std::unordered_map<std::string, std::string> AccessKey::Sign(const Request& request, time_t expires) const {
    static const std::string kExpires{"Expires"};
    static const std::string kSignature{"Signature"};
    static const std::string kAWSAccessKeyId{"AWSAccessKeyId"};

    const auto param_expires = std::to_string(expires);
    const auto string_to_sign = MakeStringToSign(request, param_expires, std::nullopt);

    std::unordered_map<std::string, std::string> sign_params{
        {kExpires, std::move(param_expires)},
        {kAWSAccessKeyId, access_key_},
        {kSignature, MakeSignature(string_to_sign, secret_key_)}};
    return sign_params;
}

}  // namespace s3api::authenticators

USERVER_NAMESPACE_END
