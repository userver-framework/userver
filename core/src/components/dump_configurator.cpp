#include <userver/components/dump_configurator.hpp>

#include <userver/components/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/components/dump_configurator.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

DumpConfigurator::DumpConfigurator(const ComponentConfig& config, const ComponentContext& context)
    : ComponentBase(config, context),
      dump_root_(config["dump-root"].As<std::string>())
{}

const std::string& DumpConfigurator::GetDumpRoot() const { return dump_root_; }

yaml_config::Schema DumpConfigurator::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/components/dump_configurator.yaml");
}

}  // namespace components

USERVER_NAMESPACE_END
