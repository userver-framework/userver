#pragma once

#include <cache/cache_config.hpp>
#include <cache/dump/operations.hpp>
#include <components/component_context.hpp>

namespace cache::dump {

/// An abstract Reader/Writer factory
class OperationsFactory {
 public:
  virtual ~OperationsFactory() = default;

  virtual std::unique_ptr<Reader> CreateReader(std::string full_path) = 0;

  virtual std::unique_ptr<Writer> CreateWriter(std::string full_path) = 0;
};

std::unique_ptr<dump::OperationsFactory> CreateOperationsFactory(
    const CacheConfigStatic& cache_config,
    const components::ComponentContext& context, const std::string& cache_name);

std::unique_ptr<dump::OperationsFactory> CreateDefaultOperationsFactory(
    const CacheConfigStatic& cache_config);

}  // namespace cache::dump
