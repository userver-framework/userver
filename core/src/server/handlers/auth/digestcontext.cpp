#include <userver/server/handlers/auth/digestcontext.hpp>
#include <userver/server/handlers/auth/digest_directives.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

DigestContextFromClient Parse(
    const userver::formats::json::Value& json,
    userver::formats::parse::To<DigestContextFromClient>) {
  return DigestContextFromClient{
      json[directives::kUsername].As<std::string>(),
      json[directives::kRealm].As<std::string>(),
      json[directives::kNonce].As<std::string>(),
      json[directives::kUri].As<std::string>(),
      json[directives::kDomain].As<std::string>(),
      json[directives::kAlgorithm].As<std::string>({}),
      json[directives::kCnonce].As<std::string>({}),
      json[directives::kOpaque].As<std::string>({}),
      json[directives::kQop].As<std::string>({}),
      json[directives::kNonceCount].As<std::string>({}),
      json[directives::kAuthParam].As<std::string>({}),
  };
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END