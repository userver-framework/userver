#include "distlock_worker.hpp"

#include <userver/testsuite/testpoint.hpp>

namespace tests::distlock {

PgWorkerComponent::PgWorkerComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::storages::postgres::DistLockComponentBase(config, context) {
  AutostartDistLock();
}

PgWorkerComponent::~PgWorkerComponent() { StopDistLock(); }

void PgWorkerComponent::DoWork() {
  TESTPOINT("distlock-worker", userver::formats::json::Value());
}

}  // namespace tests::distlock
