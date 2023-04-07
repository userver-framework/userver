#include <userver/baggage/baggage_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace baggage {

BaggageSettings BaggageSettings::Parse(
    const dynamic_config::DocsMap& docs_map) {
  BaggageSettings result{};
  result.allowed_keys = docs_map.Get("BAGGAGE_SETTINGS")["allowed_keys"]
                            .As<std::unordered_set<std::string>>();
  return result;
}

bool ParseBaggageEnabled(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("USERVER_BAGGAGE_ENABLED").As<bool>();
}

}  // namespace baggage

USERVER_NAMESPACE_END
