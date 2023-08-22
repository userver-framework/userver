#include <userver/server/handlers/auth/digest_context.hpp>
#include <userver/server/handlers/auth/digest_directives.hpp>

#include <unordered_map>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

DigestContextFromClient Parse(
    std::unordered_map<std::string, std::string> directive_mapping) {
  return DigestContextFromClient{
      directive_mapping[std::string(directives::kUsername)],
      directive_mapping[std::string(directives::kRealm)],
      directive_mapping[std::string(directives::kNonce)],
      directive_mapping[std::string(directives::kUri)],
      directive_mapping[std::string(directives::kResponse)],
      directive_mapping[std::string(directives::kAlgorithm)],
      directive_mapping[std::string(directives::kCnonce)],
      directive_mapping[std::string(directives::kOpaque)],
      directive_mapping[std::string(directives::kQop)],
      directive_mapping[std::string(directives::kNonceCount)],
      directive_mapping[std::string(directives::kAuthParam)],
  };
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
