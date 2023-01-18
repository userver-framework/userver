#include "distlock_worker.hpp"

#include <userver/testsuite/testpoint.hpp>

namespace tests::distlock {

PgWorkerComponent::PgWorkerComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : storages::postgres::DistLockComponentBase(config, context) {
  AutostartDistLock();
}

PgWorkerComponent::~PgWorkerComponent() { StopDistLock(); }

void PgWorkerComponent::DoWork() {
  TESTPOINT("distlock-worker", formats::json::Value());
}

}  // namespace tests::distlock
