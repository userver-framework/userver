#pragma once

/// @file userver/concurrent/queue_fwd.hpp
/// @brief Forward declarations for @ref concurrent::GenericQueue and its queue aliases.

#include <string>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

namespace impl {

struct NoToken;

template <typename Queue>
class QueueEmplaceEnabler;

template <bool MultipleProducer, bool MultipleConsumer>
struct SimpleQueuePolicy;

template <bool MultipleProducer, bool MultipleConsumer>
struct NoMaxSizeQueuePolicy;

template <bool MultipleProducer, bool MultipleConsumer>
struct ContainerQueuePolicy;

}  // namespace impl

template <typename T, typename QueuePolicy>
class GenericQueue;

template <typename QueueType, typename ConsumerToken, typename EmplaceEnablerType>
class Consumer;

/// @ingroup userver_concurrency
///
/// @brief Non FIFO multiple producers multiple consumers queue.
///
/// Items from the same producer are always delivered in the production order.
/// Items from different producers (or when using a `MultiProducer` token) are
/// delivered in an unspecified order. In other words, FIFO order is maintained
/// only within producers, but not between them. This may lead to increased peak
/// latency of item processing.
///
/// In exchange for this, the queue has lower contention and increased
/// throughput compared to a conventional lock-free queue.
///
/// @see @ref scripts/docs/en/userver/synchronization.md
template <typename T>
using NonFifoMpmcQueue = GenericQueue<T, impl::SimpleQueuePolicy<true, true>>;

/// @ingroup userver_concurrency
///
/// @brief Non FIFO multiple producers single consumer queue.
///
/// @see @ref concurrent::NonFifoMpmcQueue for the description of what NonFifo means.
/// @see @ref scripts/docs/en/userver/synchronization.md
template <typename T>
using NonFifoMpscQueue = GenericQueue<T, impl::SimpleQueuePolicy<true, false>>;

/// @ingroup userver_concurrency
///
/// @brief Single producer multiple consumers queue.
///
/// @see @ref scripts/docs/en/userver/synchronization.md
template <typename T>
using SpmcQueue = GenericQueue<T, impl::SimpleQueuePolicy<false, true>>;

/// @ingroup userver_concurrency
///
/// @brief Single producer single consumer queue.
///
/// @see @ref scripts/docs/en/userver/synchronization.md
template <typename T>
using SpscQueue = GenericQueue<T, impl::SimpleQueuePolicy<false, false>>;

/// @ingroup userver_concurrency
///
/// @brief Like @ref concurrent::NonFifoMpscQueue, but does not support setting max size and is thus slightly faster.
///
/// @see @ref scripts/docs/en/userver/synchronization.md
template <typename T>
using UnboundedNonFifoMpscQueue = GenericQueue<T, impl::NoMaxSizeQueuePolicy<true, false>>;

/// @ingroup userver_concurrency
///
/// @brief Like @ref concurrent::SpmcQueue, but does not support setting max size and is thus slightly faster.
///
/// @see @ref scripts/docs/en/userver/synchronization.md
template <typename T>
using UnboundedSpmcQueue = GenericQueue<T, impl::NoMaxSizeQueuePolicy<false, true>>;

/// @ingroup userver_concurrency
///
/// @brief Like @ref concurrent::SpscQueue, but does not support setting max size and is thus slightly faster.
///
/// @see @ref scripts/docs/en/userver/synchronization.md
template <typename T>
using UnboundedSpscQueue = GenericQueue<T, impl::NoMaxSizeQueuePolicy<false, false>>;

/// @ingroup userver_concurrency
///
/// @brief Single producer single consumer queue of std::string which is bounded by the total bytes inside the strings.
///
/// @see @ref scripts/docs/en/userver/synchronization.md
using StringStreamQueue = GenericQueue<std::string, impl::ContainerQueuePolicy<false, false>>;

/// @brief Consumer of @ref concurrent::StringStreamQueue.
using StringStreamQueueConsumer =
    Consumer<StringStreamQueue, impl::NoToken, impl::QueueEmplaceEnabler<StringStreamQueue>>;

}  // namespace concurrent

USERVER_NAMESPACE_END
