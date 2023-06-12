#include "dynamic_config.hpp"

#include <userver/components/component.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/testsuite/testpoint.hpp>

namespace tests::handlers {

namespace {

int ParseMeaningOfLife(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("MEANING_OF_LIFE").As<int>();
}

constexpr dynamic_config::Key<ParseMeaningOfLife> kMeaningOfLife;

}  // namespace

DynamicConfig::DynamicConfig(const components::ComponentConfig& config,
                             const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      config_source_(
          context.FindComponent<components::DynamicConfig>().GetSource()) {}

std::string DynamicConfig::HandleRequestThrow(
    [[maybe_unused]] const server::http::HttpRequest& request,
    [[maybe_unused]] server::request::RequestContext& context) const {
  const int config_value = config_source_.GetCopy(kMeaningOfLife);
  return std::to_string(config_value);
}

}  // namespace tests::handlers
