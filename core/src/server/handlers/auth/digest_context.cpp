#include <userver/server/handlers/auth/digest_context.hpp>
#include <userver/server/handlers/auth/digest_directives.hpp>

#include <unordered_map>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

DigestContextFromClient Parse(
    std::unordered_map<std::string, std::string> directive_mapping) {
  return DigestContextFromClient{
      directive_mapping[std::string(directives::kUsername.data())],
      directive_mapping[std::string(directives::kRealm.data())],
      directive_mapping[std::string(directives::kNonce.data())],
      directive_mapping[std::string(directives::kUri.data())],
      directive_mapping[std::string(directives::kResponse.data())],
      directive_mapping[std::string(directives::kAlgorithm.data())],
      directive_mapping[std::string(directives::kCnonce.data())],
      directive_mapping[std::string(directives::kOpaque.data())],
      directive_mapping[std::string(directives::kQop.data())],
      directive_mapping[std::string(directives::kNonceCount.data())],
      directive_mapping[std::string(directives::kAuthParam.data())],
  };
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
