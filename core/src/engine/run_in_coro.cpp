#include <engine/run_in_coro.hpp>

#include <utility>

#include <engine/run_standalone.hpp>

void RunInCoro(std::function<void()> payload, std::size_t worker_threads) {
  engine::RunStandalone(worker_threads, std::move(payload));
}
