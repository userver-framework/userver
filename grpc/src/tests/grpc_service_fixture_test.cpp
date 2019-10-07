#include "grpc_service_fixture_test.hpp"

#include <engine/async.hpp>
#include <tracing/span.hpp>

namespace clients::grpc::test {

TestWithTaskProcessor::TestWithTaskProcessor()
    : task_processor_{engine::impl::TaskProcessorHolder::MakeTaskProcessor(
          1, "test_processor", engine::impl::MakeTaskProcessorPools())} {}

void TestWithTaskProcessor::RunTestInCoro(std::function<void()> user_cb) {
  engine::impl::RunOnTaskProcessorSync(*task_processor_, user_cb);
}
}  // namespace clients::grpc::test
