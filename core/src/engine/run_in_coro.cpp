#include <userver/engine/run_in_coro.hpp>

#include <utility>

#include <userver/engine/run_standalone.hpp>

USERVER_NAMESPACE_BEGIN

void RunInCoro(utils::function_ref<void()> payload,
               std::size_t worker_threads) {
  engine::RunStandalone(worker_threads, std::move(payload));
}

USERVER_NAMESPACE_END
