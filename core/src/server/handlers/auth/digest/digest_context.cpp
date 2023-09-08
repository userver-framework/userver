#include <userver/server/handlers/auth/digest/digest_context.hpp>
#include <userver/server/handlers/auth/digest/digest_directives.hpp>

#include <unordered_map>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest {

ContextFromClient Parse(
    std::unordered_map<std::string, std::string> directive_mapping) {
  return ContextFromClient{
      directive_mapping[directives::kUsername],
      directive_mapping[directives::kRealm],
      directive_mapping[directives::kNonce],
      directive_mapping[directives::kUri],
      directive_mapping[directives::kResponse],
      directive_mapping[directives::kAlgorithm],
      directive_mapping[directives::kCnonce],
      directive_mapping[directives::kOpaque],
      directive_mapping[directives::kQop],
      directive_mapping[directives::kNonceCount],
      directive_mapping[directives::kAuthParam],
  };
}

}  // namespace server::handlers::auth::digest

USERVER_NAMESPACE_END
