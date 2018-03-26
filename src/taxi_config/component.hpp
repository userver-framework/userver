#pragma once

#include <chrono>

#include <components/caching_component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>

#include "config.hpp"

namespace components {

class TaxiConfig : public CachingComponentBase<taxi_config::TaxiConfig> {
 public:
  static constexpr const char* kName = "taxi-config";

  TaxiConfig(const ComponentConfig&, const ComponentContext&);
  ~TaxiConfig();

 private:
  void Update(UpdateType type,
              const std::chrono::system_clock::time_point& last_update,
              const std::chrono::system_clock::time_point& now) override;

  std::chrono::system_clock::time_point seen_doc_update_time_;
  storages::mongo::PoolPtr mongo_taxi_;
  std::string fallback_path_;
};

}  // namespace components
