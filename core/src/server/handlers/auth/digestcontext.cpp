#include <userver/server/handlers/auth/digestcontext.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

DigestContextFromClient Parse(
    const userver::formats::json::Value& json,
    userver::formats::parse::To<DigestContextFromClient>) {
  return DigestContextFromClient{
      json["username"].As<std::string>(),
      json["realm"].As<std::string>(),
      json["nonce"].As<std::string>(),
      json["uri"].As<std::string>(),
      json["response"].As<std::string>(),
      json["algorithm"].As<std::string>({}),
      json["cnonce"].As<std::string>({}),
      json["opaque"].As<std::string>({}),
      json["qop"].As<std::string>({}),
      json["nc"].As<std::string>({}),
      json["auth-param"].As<std::string>({}),
  };
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END