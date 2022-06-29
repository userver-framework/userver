#include "distlock_worker.hpp"

#include <userver/components/component_context.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/testsuite/testpoint.hpp>

namespace tests::distlock {

MongoWorkerComponent::MongoWorkerComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::storages::mongo::DistLockComponentBase(
          config, context,
          context.FindComponent<userver::components::Mongo>("mongo-sample")
              .GetPool()
              ->GetCollection("distlocks")) {
  Start();
}

MongoWorkerComponent::~MongoWorkerComponent() { Stop(); }

void MongoWorkerComponent::DoWork() {
  TESTPOINT("distlock-worker", userver::formats::json::Value());
}

}  // namespace tests::distlock
