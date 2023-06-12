#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/storages/mongo/dist_lock_component_base.hpp>

namespace tests::distlock {

class MongoWorkerComponent final
    : public storages::mongo::DistLockComponentBase {
 public:
  static constexpr std::string_view kName = "distlock-mongo";

  MongoWorkerComponent(const components::ComponentConfig& config,
                       const components::ComponentContext& context);
  ~MongoWorkerComponent() override;

  void DoWork() final;
};

}  // namespace tests::distlock
