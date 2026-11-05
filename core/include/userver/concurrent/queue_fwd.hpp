#pragma once

/// @file userver/concurrent/queue_fwd.hpp
/// @brief Forward declarations for @ref concurrent::GenericQueue and @ref concurrent::StringStreamQueue

#include <string>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

namespace impl {

template <bool MultipleProducer, bool MultipleConsumer>
struct ContainerQueuePolicy;

}  // namespace impl

template <typename T, typename QueuePolicy>
class GenericQueue;

/// @ingroup userver_concurrency
///
/// @brief Single producer single consumer queue of std::string which is bounded by the total bytes inside the strings.
///
/// @see @ref scripts/docs/en/userver/synchronization.md
using StringStreamQueue = GenericQueue<std::string, impl::ContainerQueuePolicy<false, false>>;

}  // namespace concurrent

USERVER_NAMESPACE_END
