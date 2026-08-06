#include "dataset_provider.hpp"
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace userver_httparena {
DatasetProvider::DatasetProvider(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ComponentBase(config, context)
{
    const auto path = config["dataset-path"].As<std::string>();
    const auto doc = formats::json::blocking::FromFile(path);
    for (const auto& item_json : doc) {
        items_.push_back(item_json.As<Item>());
    }
}

yaml_config::Schema DatasetProvider::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<components::ComponentBase>(
        R"(
type: object
description: Dataset provider component
additionalProperties: false
properties:
    dataset-path:
        type: string
        description: path to the JSON dataset file
)"
    );
}
}  // namespace userver_httparena
