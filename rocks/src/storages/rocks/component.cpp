#include <userver/storages/rocks/component.hpp>

#include <memory>

#include <userver/storages/rocks/client.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/storages/rocks/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

namespace {

TransactionType ParseTransactionType(const std::string& value) {
    if (value == "pessimistic") {
        return TransactionType::kPessimistic;
    }
    if (value == "optimistic") {
        return TransactionType::kOptimistic;
    }
    return TransactionType::kNone;
}

}  // namespace

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ComponentBase(config, context),
      client_ptr_(std::make_shared<storages::rocks::Client>(
          config["db-path"].As<std::string>(),
          ParseTransactionType(config["transaction-type"].As<std::string>("none"))
      ))
{}

storages::rocks::ClientPtr Component::MakeClient() { return client_ptr_; }

yaml_config::Schema Component::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/storages/rocks/component.yaml");
}

}  // namespace storages::rocks

USERVER_NAMESPACE_END
