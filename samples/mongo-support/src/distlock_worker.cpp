#include "distlock_worker.hpp"

#include <userver/components/component_context.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/testsuite/testpoint.hpp>

namespace tests::distlock {

MongoWorkerComponent::MongoWorkerComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : storages::mongo::DistLockComponentBase(
          config, context,
          context.FindComponent<components::Mongo>("mongo-sample")
              .GetPool()
              ->GetCollection("distlocks")) {
  Start();
}

MongoWorkerComponent::~MongoWorkerComponent() { Stop(); }

void MongoWorkerComponent::DoWork() {
  TESTPOINT("distlock-worker", formats::json::Value());
}

}  // namespace tests::distlock
