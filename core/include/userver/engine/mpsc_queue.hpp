#pragma once

#include <userver/concurrent/mpsc_queue.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {
template <typename T>
using MpscQueue = concurrent::MpscQueue<T>;
}  // namespace engine

USERVER_NAMESPACE_END
