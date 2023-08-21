#include <userver/server/handlers/auth/digest_context.hpp>
#include <userver/server/handlers/auth/digest_directives.hpp>

#include <unordered_map>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

DigestContextFromClient Parse(
    std::unordered_map<std::string, std::string> directive_mapping) {
  return DigestContextFromClient{
      directive_mapping[directives::kUsername.data()],
      directive_mapping[directives::kRealm.data()],
      directive_mapping[directives::kNonce.data()],
      directive_mapping[directives::kUri.data()],
      directive_mapping[directives::kResponse.data()],
      directive_mapping[directives::kAlgorithm.data()],
      directive_mapping[directives::kCnonce.data()],
      directive_mapping[directives::kOpaque.data()],
      directive_mapping[directives::kQop.data()],
      directive_mapping[directives::kNonceCount.data()],
      directive_mapping[directives::kAuthParam.data()],
  };
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END