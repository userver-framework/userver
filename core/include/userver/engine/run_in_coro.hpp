#pragma once

/// @file userver/engine/run_in_coro.hpp
/// @brief @copybrief RunInCoro

#include <cstddef>
#include <functional>

USERVER_NAMESPACE_BEGIN

/// Deprecated, use engine::RunStandalone instead
void RunInCoro(std::function<void()> payload, std::size_t worker_threads = 1);

USERVER_NAMESPACE_END
