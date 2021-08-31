#pragma once

#include <userver/concurrent/mpsc_queue.hpp>

namespace engine {
template <typename T>
using MpscQueue = concurrent::MpscQueue<T>;
}  // namespace engine
