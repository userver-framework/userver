#pragma once

#include <chrono>
#include <functional>
#include <string>

#include <yandex/taxi/userver/components/component_config.hpp>
#include <yandex/taxi/userver/components/component_context.hpp>
#include <yandex/taxi/userver/components/updating_component_base.hpp>
#include <yandex/taxi/userver/storages/mongo/pool.hpp>

#include <yandex/taxi/userver/taxi_config/value.hpp>

namespace components {

class TaxiConfigImpl {
 public:
  using EmplaceDocsCb = std::function<void(taxi_config::DocsMap&& mongo_docs)>;

  TaxiConfigImpl(const ComponentConfig&, const ComponentContext&,
                 EmplaceDocsCb emplace_docs_cb);

  void Update(UpdatingComponentBase::UpdateType type,
              const std::chrono::system_clock::time_point& last_update,
              const std::chrono::system_clock::time_point& now);

 private:
  const EmplaceDocsCb emplace_docs_cb_;
  std::chrono::system_clock::time_point seen_doc_update_time_;
  storages::mongo::PoolPtr mongo_taxi_;
  std::string fallback_path_;
};

}  // namespace components
