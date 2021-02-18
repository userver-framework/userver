#pragma once

#include <functional>
#include <optional>

void RunInCoro(std::function<void()>, size_t worker_threads = 1,
               std::optional<size_t> initial_coro_pool_size = {},
               std::optional<size_t> max_coro_pool_size = {});
