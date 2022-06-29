#pragma once

#include <userver/storages/mongo/dist_lock_component_base.hpp>

namespace tests::distlock {

class MongoWorkerComponent final
    : public userver::storages::mongo::DistLockComponentBase {
 public:
  static constexpr std::string_view kName = "distlock-mongo";

  MongoWorkerComponent(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context);
  ~MongoWorkerComponent() override;

  void DoWork() final;
};

}  // namespace tests::distlock
