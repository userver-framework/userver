#pragma once

/// @file userver/s3api/authenticators/signature_v4.hpp
/// @brief Authenticator implementing AWS Signature Version 4

#include <string>
#include <unordered_map>

#include <userver/s3api/authenticators/interface.hpp>
#include <userver/s3api/models/fwd.hpp>
#include <userver/s3api/models/secret.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::authenticators {

/// @brief Authenticator implementing AWS Signature Version 4.
///
/// `Auth` signs a request with headers (`Authorization`, `X-Amz-Date`,
/// `X-Amz-Content-Sha256`), `Sign` produces query parameters for a presigned
/// URL (`X-Amz-Algorithm`, `X-Amz-Credential`, `X-Amz-Date`, `X-Amz-Expires`,
/// `X-Amz-SignedHeaders`, `X-Amz-Signature`).
///
/// See https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-authenticating-requests.html
class SignatureV4 : public Authenticator {
public:
    SignatureV4(std::string access_key, Secret secret_key, std::string region, std::string service = "s3")
        : access_key_{std::move(access_key)},
          secret_key_{std::move(secret_key)},
          region_{std::move(region)},
          service_{std::move(service)}
    {}

    std::unordered_map<std::string, std::string> Auth(const Request& request) const override;

    /// @note `expires` is an absolute unix timestamp of the moment the
    /// presigned URL stops being valid, the same way as in
    /// @ref AccessKey::Sign. It is converted to the `X-Amz-Expires` duration
    /// relative to the current time.
    std::unordered_map<std::string, std::string> Sign(const Request& request, std::time_t expires) const override;

private:
    std::string access_key_;
    Secret secret_key_;
    std::string region_;
    std::string service_;
};

}  // namespace s3api::authenticators

USERVER_NAMESPACE_END
