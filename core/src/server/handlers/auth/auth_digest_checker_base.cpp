#include <userver/server/handlers/auth/auth_digest_checker_base.hpp>

#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include <fmt/core.h>
#include <fmt/format.h>

#include <userver/crypto/algorithm.hpp>
#include <userver/crypto/hash.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/server/handlers/auth/digest_directives.hpp>
#include <userver/server/handlers/auth/digest_types.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/handlers/fallback_handlers.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

constexpr std::string_view kDigestWord = "Digest";

constexpr std::string_view kAuthenticationInfo = "Authentication-Info";
constexpr std::string_view kProxyAuthenticationInfo =
    "Proxy-Authentication-Info";

DigestHasher::DigestHasher(const std::string& algorithm) {
  switch (
      kHashAlgToType.TryFindICase(algorithm).value_or(HashAlgTypes::kUnknown)) {
    case HashAlgTypes::kMD5:
      hash_algorithm_ = &crypto::hash::weak::Md5;
      break;
    case HashAlgTypes::kSHA256:
      hash_algorithm_ = &crypto::hash::Sha256;
      break;
    case HashAlgTypes::kSHA512:
      hash_algorithm_ = &crypto::hash::Sha512;
      break;
    default:
      throw std::runtime_error("Unknown hash algorithm");
  }
}

std::string DigestHasher::Nonce() const {
  return GetHash(std::to_string(
      std::chrono::system_clock::now().time_since_epoch().count()));
}
std::string DigestHasher::GetHash(std::string_view data) const {
  return hash_algorithm_(data, crypto::hash::OutputEncoding::kHex);
}

AuthCheckerDigestBase::AuthCheckerDigestBase(
    const AuthDigestSettings& digest_settings, std::string&& realm)
    : qops_(fmt::format("{}", fmt::join(digest_settings.qops, ","))),
      realm_(std::move(realm)),
      domains_(fmt::format("{}", fmt::join(digest_settings.domains, ", "))),
      algorithm_(digest_settings.algorithm),
      is_session_(digest_settings.is_session),
      is_proxy_(digest_settings.is_proxy),
      nonce_ttl_(digest_settings.nonce_ttl),
      digest_hasher_(algorithm_),
      authenticate_header_(is_proxy_
                               ? userver::http::headers::kProxyAuthenticate
                               : userver::http::headers::kWWWAuthenticate),
      authorization_header_(is_proxy_
                                ? userver::http::headers::kProxyAuthorization
                                : userver::http::headers::kAuthorization),
      authenticate_info_header_(is_proxy_ ? kProxyAuthenticationInfo
                                          : kAuthenticationInfo),
      unauthorized_status_(is_proxy_
                               ? http::HttpStatus::kProxyAuthenticationRequired
                               : http::HttpStatus::kUnauthorized) {}

AuthCheckResult AuthCheckerDigestBase::CheckAuth(
    const http::HttpRequest& request, request::RequestContext&) const {
  // RFC 2617, 3
  // Digest Access Authentication.
  auto& response = request.GetHttpResponse();

  const auto& auth_value = request.GetHeader(authorization_header_);
  if (auth_value.empty()) {
    // If there is no authorization header, we save the "nonce" to temporary
    // storage.
    auto nonce = digest_hasher_.Nonce();
    PushUnnamedNonce(nonce, nonce_ttl_);

    response.SetStatus(unauthorized_status_);
    response.SetHeader(authenticate_header_,
                       ConstructResponseDirectives(nonce, false));
    return AuthCheckResult{AuthCheckResult::Status::kInvalidToken};
  }

  DigestParsing parser;
  parser.ParseAuthInfo(auth_value.substr(kDigestWord.size() + 1));
  const auto& client_context = parser.GetClientContext();

  // Check if user have been registred.
  auto user_data_opt = GetUserData(client_context.username);
  if (!user_data_opt.has_value()) {
    return AuthCheckResult{AuthCheckResult::Status::kForbidden};
  }
  const auto& user_data = user_data_opt.value();

  auto validate_result = ValidateUserData(client_context, user_data);
  switch (validate_result) {
    case ValidateResult::kWrongUserData:
      return StartNewAuthSession(client_context.username,
                                 digest_hasher_.Nonce(), true, response);
    case ValidateResult::kDuplicateRequest:
      response.SetStatus(unauthorized_status_);
      return AuthCheckResult{AuthCheckResult::Status::kTokenNotFound};
    case ValidateResult::kOk:
      break;
  }

  auto digest =
      CalculateDigest(user_data.ha1, request.GetMethod(), client_context);

  if (!crypto::algorithm::AreStringsEqualConstTime(digest,
                                                   client_context.response)) {
    response.SetStatus(unauthorized_status_);
    response.SetHeader(authenticate_header_, ConstructResponseDirectives(
                                                 client_context.nonce, false));
    return AuthCheckResult{AuthCheckResult::Status::kInvalidToken};
  }

  // RFC 2617, 3.2.3
  // Authentication-Info contains the "nextnonce" required for subsequent
  // authentication.
  auto info_header_directives = ConstructAuthInfoHeader(client_context);
  response.SetHeader(authenticate_info_header_, info_header_directives);

  return {};
};

ValidateResult AuthCheckerDigestBase::ValidateUserData(
    const DigestContextFromClient& client_context,
    const UserData& user_data) const {
  bool are_nonces_equal = crypto::algorithm::AreStringsEqualConstTime(
      user_data.nonce, client_context.nonce);
  bool is_nonce_expired =
      user_data.timestamp + nonce_ttl_ < userver::utils::datetime::Now();
  if (!are_nonces_equal) {
    // "nonce" may be in temporary storage.
    auto nonce_creation_time =
        GetUnnamedNonceCreationTime(client_context.nonce);
    if (!nonce_creation_time.has_value()) {
      return ValidateResult::kWrongUserData;
    }

    SetUserData(client_context.username, client_context.nonce, 0,
                nonce_creation_time.value());
  }

  if (is_nonce_expired) {
    return ValidateResult::kWrongUserData;
  }

  LOG_DEBUG() << "Nonce is OK";

  auto client_nc = std::stoul(client_context.nc, nullptr, 16);
  if (user_data.nonce_count < client_nc) {
    SetUserData(client_context.username, client_context.nonce, 0,
                utils::datetime::Now());
  } else {
    return ValidateResult::kDuplicateRequest;
  }
  LOG_DEBUG() << "Nonce_count is OK";

  return ValidateResult::kOk;
}

std::string AuthCheckerDigestBase::ConstructAuthInfoHeader(
    const DigestContextFromClient& client_context) const {
  auto next_nonce = digest_hasher_.Nonce();

  SetUserData(client_context.username, next_nonce, 0, utils::datetime::Now());

  return fmt::format("{}=\"{}\"", directives::kNextNonce, next_nonce);
}

AuthCheckResult AuthCheckerDigestBase::StartNewAuthSession(
    const std::string& username, const std::string& nonce_from_client,
    bool stale, http::HttpResponse& response) const {
  SetUserData(username, nonce_from_client, 0, utils::datetime::Now());
  response.SetStatus(unauthorized_status_);
  response.SetHeader(authenticate_header_,
                     ConstructResponseDirectives(nonce_from_client, stale));

  return AuthCheckResult{AuthCheckResult::Status::kInvalidToken};
}

// clang-format off
std::string AuthCheckerDigestBase::ConstructResponseDirectives(
    std::string_view nonce, bool stale) const {
  // RFC 2617, 3.2.1
  // Server response directives.
  return utils::StrCat(
      "Digest ",
      fmt::format("{}=\"{}\", ", directives::kRealm, realm_),
      fmt::format("{}=\"{}\", ", directives::kNonce, nonce),
      fmt::format("{}=\"{}\", ", directives::kStale, stale),
      fmt::format("{}=\"{}\", ", directives::kDomain, domains_),
      fmt::format("{}=\"{}\", ", directives::kAlgorithm, algorithm_),
      fmt::format("{}=\"{}\"", directives::kQop, qops_));
}
// clang-format on

std::string AuthCheckerDigestBase::CalculateDigest(
    const UserData::HA1& ha1_non_loggable, http::HttpMethod request_method,
    const DigestContextFromClient& client_context) const {
  // RFC 2617, 3.2.2.1 Request-Digest

  auto ha1 = ha1_non_loggable.GetUnderlying();
  if (is_session_) {
    ha1 = fmt::format("{}:{}:{}", ha1, client_context.nonce,
                      client_context.cnonce);
  }

  auto a2 = fmt::format("{}:{}", ToString(request_method), client_context.uri);
  std::string ha2 = digest_hasher_.GetHash(a2);

  std::string request_digest = fmt::format(
      "{}:{}:{}:{}:{}:{}", ha1, client_context.nonce, client_context.nc,
      client_context.cnonce, client_context.qop, ha2);
  return digest_hasher_.GetHash(request_digest);
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END