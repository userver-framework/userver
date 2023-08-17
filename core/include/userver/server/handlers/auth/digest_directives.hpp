#pragma once

#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

enum class DirectiveTypes {
  kRealm,
  kNonce,
  kStale,
  kDomain,
  kOpaque,
  kAlgorithm,
  kQop
};

constexpr utils::TrivialBiMap kTypesToDirectives = [](auto selector) {
  return selector()
      .Case("realm", DirectiveTypes::kRealm)
      .Case("nonce", DirectiveTypes::kNonce)
      .Case("stale", DirectiveTypes::kStale)
      .Case("domain", DirectiveTypes::kDomain)
      .Case("opaque", DirectiveTypes::kOpaque)
      .Case("algorithm", DirectiveTypes::kAlgorithm)
      .Case("qop", DirectiveTypes::kQop);
};

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END