#pragma once
#include <concurrent/async_event_channel.hpp>

namespace utils {
using AsyncEventChannelBase = concurrent::AsyncEventChannelBase;
using AsyncEventSubscriberScope = concurrent::AsyncEventSubscriberScope;

template <typename... Args>
using AsyncEventChannel = concurrent::AsyncEventChannel<Args...>;
}  // namespace utils
