#pragma once

#include <components/component_context.hpp>
#include <dump/config.hpp>
#include <dump/operations.hpp>
#include <utils/prof.hpp>

namespace dump {

/// An abstract Reader/Writer factory
class OperationsFactory {
 public:
  virtual ~OperationsFactory() = default;

  virtual std::unique_ptr<Reader> CreateReader(std::string full_path) = 0;

  virtual std::unique_ptr<Writer> CreateWriter(std::string full_path,
                                               ScopeTime& scope) = 0;
};

std::unique_ptr<dump::OperationsFactory> CreateOperationsFactory(
    const Config& cache_config, const components::ComponentContext& context,
    const std::string& cache_name);

std::unique_ptr<dump::OperationsFactory> CreateDefaultOperationsFactory(
    const Config& cache_config);

}  // namespace dump
