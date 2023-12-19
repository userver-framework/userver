#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/storages/postgres/dist_lock_component_base.hpp>

namespace tests::distlock {

class PgWorkerComponent final
    : public storages::postgres::DistLockComponentBase {
 public:
  static constexpr std::string_view kName = "distlock-pg";

  PgWorkerComponent(const components::ComponentConfig& config,
                    const components::ComponentContext& context);
  ~PgWorkerComponent() override;

  void DoWork() final;
};

}  // namespace tests::distlock
