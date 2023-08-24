#pragma once

namespace moodycamel {
struct ConcurrentQueueDefaultTraits;

template <typename T, typename Traits>
class ConcurrentQueue;
}  // namespace moodycamel
