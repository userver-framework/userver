#include <userver/components/dump_configurator.hpp>

#include <userver/components/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

DumpConfigurator::DumpConfigurator(const ComponentConfig& config,
                                   const ComponentContext& context)
    : LoggableComponentBase(config, context),
      dump_root_(config["dump-root"].As<std::string>()) {}

const std::string& DumpConfigurator::GetDumpRoot() const { return dump_root_; }

std::string DumpConfigurator::GetStaticConfigSchema() {
  return R"(
type: object
description: dump-configurator schema
additionalProperties: false
properties:
    dump-root:
        type: string
        description: Components store dumps in subdirectories of this directory
)";
}

}  // namespace components

USERVER_NAMESPACE_END
