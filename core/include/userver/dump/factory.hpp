#pragma once

#include <userver/components/component_context.hpp>
#include <userver/dump/config.hpp>
#include <userver/dump/operations.hpp>
#include <userver/tracing/scope_time.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

/// An abstract Reader/Writer factory
class OperationsFactory {
 public:
  virtual ~OperationsFactory() = default;

  virtual std::unique_ptr<Reader> CreateReader(std::string full_path) = 0;

  virtual std::unique_ptr<Writer> CreateWriter(std::string full_path,
                                               tracing::ScopeTime& scope) = 0;
};

std::unique_ptr<dump::OperationsFactory> CreateOperationsFactory(
    const Config& config, const components::ComponentContext& context);

std::unique_ptr<dump::OperationsFactory> CreateDefaultOperationsFactory(
    const Config& config);

}  // namespace dump

USERVER_NAMESPACE_END
