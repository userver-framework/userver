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
#include <userver/server/handlers/auth/digest_algorithms.hpp>
#include <userver/server/handlers/auth/digest_directives.hpp>
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

DigestHasher::DigestHasher(const Algorithm& algorithm) {
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
    const AuthDigestSettings& digest_settings, Realm&& realm)
    : qops_(digest_settings.qops),
      qops_str_(fmt::format("{}", fmt::join(qops_, ","))),
      realm_(std::move(realm)),
      domains_str_(fmt::format("{}", fmt::join(digest_settings.domains, ", "))),
      algorithm_(digest_settings.algorithm),
      is_session_(digest_settings.is_session.value_or(false)),
      is_proxy_(digest_settings.is_proxy.value_or(false)),
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
      unauthorized_status_(
          is_proxy_ ? server::http::HttpStatus::kProxyAuthenticationRequired
                    : server::http::HttpStatus::kUnauthorized) {}

AuthCheckResult AuthCheckerDigestBase::CheckAuth(
    const server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  // RFC 2617, 3
  // Digest Access Authentication.
  auto& response = request.GetHttpResponse();

  const auto& auth_value = request.GetHeader(authorization_header_);
  if (auth_value.empty()) {
    // If there is no authorization header, we save the "nonce" to temporary
    // storage.
    auto nonce = digest_hasher_.Nonce();
    PushUnnamedNonce(nonce);

    response.SetStatus(unauthorized_status_);
    response.SetHeader(authenticate_header_,
                       ConstructResponseDirectives(nonce, false));
    return AuthCheckResult{AuthCheckResult::Status::kInvalidToken};
  }

  DigestParsing parser;
  parser.ParseAuthInfo(auth_value.substr(kDigestWord.size() + 1));
  const auto& client_context = parser.GetClientContext();

  auto validate_result = ValidateClientData(client_context);
  switch (validate_result) {
    case ValidateClientDataResult::kWrongUserData:
      return StartNewAuthSession(client_context.username,
                                 digest_hasher_.Nonce(), true, response);
    case ValidateClientDataResult::kUserNotRegistred:
      response.SetStatus(unauthorized_status_);
      return AuthCheckResult{AuthCheckResult::Status::kTokenNotFound};
    case ValidateClientDataResult::kOk:
      break;
  }

  auto digest = CalculateDigest(request.GetMethod(), client_context);
  if (!digest.has_value()) {
    return AuthCheckResult{AuthCheckResult::Status::kForbidden};
  }

  if (!crypto::algorithm::AreStringsEqualConstTime(digest.value(),
                                                   client_context.response)) {
    response.SetStatus(unauthorized_status_);
    response.SetHeader(authenticate_header_, ConstructResponseDirectives(
                                                 client_context.nonce, false));
    return AuthCheckResult{AuthCheckResult::Status::kInvalidToken};
  }

  // RFC 2617, 3.2.3
  // Authentication-Info contains the "nextnonce" required for subsequent authentication.
  auto info_header_directives = ConstructAuthInfoHeader(client_context);
  response.SetHeader(authenticate_info_header_, info_header_directives);

  return {};
};

ValidateClientDataResult AuthCheckerDigestBase::ValidateClientData(
    const DigestContextFromClient& client_context) const {
  auto user_data_opt = GetUserData(client_context.username);
  if (!user_data_opt.has_value()) {
    // If the user is not found, his "nonce" may be in temporary storage.
    if (!HasUnnamedNonce(client_context.nonce)) {
      return ValidateClientDataResult::kWrongUserData;
    }

    UserData user_data{client_context.nonce, utils::datetime::Now()};
    SetUserData(client_context.username, user_data);
    user_data_opt = user_data;
  }

  const auto& user_data = user_data_opt.value();

  if (IsNonceExpired(client_context.nonce, user_data)) {
    return ValidateClientDataResult::kWrongUserData;
  }
  LOG_DEBUG() << "NONCE IS OK";

  auto client_nc = std::stoul(client_context.nc, nullptr, 16);
  if (user_data.nonce_count < client_nc) {
    UserData user_data{client_context.nonce, utils::datetime::Now()};
    SetUserData(client_context.username, std::move(user_data));
  } else {
    return ValidateClientDataResult::kUserNotRegistred;
  }
  LOG_DEBUG() << "NONCE_COUNT IS OK";

  return ValidateClientDataResult::kOk;
}

std::string AuthCheckerDigestBase::ConstructAuthInfoHeader(
    const DigestContextFromClient& client_context) const {
  auto next_nonce = digest_hasher_.Nonce();

  UserData user_data{next_nonce, userver::utils::datetime::Now()};
  SetUserData(client_context.username, std::move(user_data));

  return fmt::format("{}=\"{}\"", directives::kNextNonce, next_nonce);
}

AuthCheckResult AuthCheckerDigestBase::StartNewAuthSession(
    const std::string& username, const std::string& nonce_from_client,
    bool stale, server::http::HttpResponse& response) const {
  UserData user_data{nonce_from_client, userver::utils::datetime::Now()};
  SetUserData(username, std::move(user_data));
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
      fmt::format("{}=\"{}\", ", directives::kDomain, domains_str_),
      fmt::format("{}=\"{}\", ", directives::kAlgorithm, algorithm_),
      fmt::format("{}=\"{}\"", directives::kQop, qops_str_));
}
// clang-format on

bool AuthCheckerDigestBase::IsNonceExpired(std::string_view nonce_from_client,
                                           const UserData& user_data) const {
  LOG_DEBUG() << "NONCE: " << user_data.nonce;
  LOG_DEBUG() << "NONCE_FROM_CLIENT: " << nonce_from_client;
  if (!crypto::algorithm::AreStringsEqualConstTime(user_data.nonce,
                                                   nonce_from_client)) {
    return true;
  }

  return user_data.timestamp + nonce_ttl_ < userver::utils::datetime::Now();
}

std::optional<std::string> AuthCheckerDigestBase::CalculateDigest(
    const server::http::HttpMethod& request_method,
    const DigestContextFromClient& client_context) const {
  // RFC 2617, 3.2.2.1 Request-Digest
  auto ha1_opt = GetHA1(client_context.username);
  if (!ha1_opt.has_value()) {
    return std::nullopt;
  }

  auto ha1 = ha1_opt.value().GetUnderlying();
  if (is_session_) {
    ha1 = fmt::format("{}:{}:{}", ha1, client_context.nonce,
                      client_context.cnonce);
  }

  auto a2 = fmt::format("{}:{}", ToString(request_method), client_context.uri);
  std::string ha2 = digest_hasher_.GetHash(a2);

  // request_digest = H(HA1:nonce:nc:cnonce:qop:HA2).
  std::string request_digest = fmt::format(
      "{}:{}:{}:{}:{}:{}", ha1, client_context.nonce, client_context.nc,
      client_context.cnonce, client_context.qop, ha2);
  return digest_hasher_.GetHash(request_digest);
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
