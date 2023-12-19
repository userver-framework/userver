#pragma once

/// @file userver/engine/run_in_coro.hpp
/// @brief @copybrief RunInCoro

#include <cstddef>

#include <userver/utils/function_ref.hpp>

USERVER_NAMESPACE_BEGIN

/// @deprecated use engine::RunStandalone instead
void RunInCoro(utils::function_ref<void()> payload,
               std::size_t worker_threads = 1);

USERVER_NAMESPACE_END
