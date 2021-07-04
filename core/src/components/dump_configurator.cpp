#include <userver/components/dump_configurator.hpp>

#include <userver/components/component_config.hpp>

namespace components {

DumpConfigurator::DumpConfigurator(const ComponentConfig& config,
                                   const ComponentContext& context)
    : LoggableComponentBase(config, context),
      dump_root_(config["dump-root"].As<std::string>()) {}

const std::string& DumpConfigurator::GetDumpRoot() const { return dump_root_; }

}  // namespace components
