#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/yaml_config/schema.hpp>

#include <schemas/types.hpp>

namespace userver_httparena {
class DatasetProvider final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "dataset-provider";

    DatasetProvider(const components::ComponentConfig& config, const components::ComponentContext& context);

    static constexpr auto kConfigFileMode = components::ConfigFileMode::kNotRequired;

    static yaml_config::Schema GetStaticConfigSchema();

    const std::vector<Item>& GetItems() const { return items_; }

private:
    std::vector<Item> items_;
};
}  // namespace userver_httparena
