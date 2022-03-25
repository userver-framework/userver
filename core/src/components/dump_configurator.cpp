#include <userver/components/dump_configurator.hpp>

#include <userver/components/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

DumpConfigurator::DumpConfigurator(const ComponentConfig& config,
                                   const ComponentContext& context)
    : LoggableComponentBase(config, context),
      dump_root_(config["dump-root"].As<std::string>()) {}

const std::string& DumpConfigurator::GetDumpRoot() const { return dump_root_; }

yaml_config::Schema DumpConfigurator::GetStaticConfigSchema() {
  yaml_config::Schema schema(R"(
type: object
description: dump-configurator schema
additionalProperties: false
properties:
    dump-root:
        type: string
        description: Components store dumps in subdirectories of this directory
)");
  yaml_config::Merge(schema, LoggableComponentBase::GetStaticConfigSchema());
  return schema;
}

}  // namespace components

USERVER_NAMESPACE_END
