#pragma once

#include <userver/storages/postgres/dist_lock_component_base.hpp>

namespace tests::distlock {

class PgWorkerComponent final
    : public userver::storages::postgres::DistLockComponentBase {
 public:
  static constexpr std::string_view kName = "distlock-pg";

  PgWorkerComponent(const userver::components::ComponentConfig& config,
                    const userver::components::ComponentContext& context);
  ~PgWorkerComponent() override;

  void DoWork() final;
};

}  // namespace tests::distlock
