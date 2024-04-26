#pragma once

#include <memory>
#include <type_traits>

#include <ares.h>

#include <userver/compiler/thread_local.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns::impl {

class GlobalInitializer {
 public:
  GlobalInitializer();
  ~GlobalInitializer();
};

struct ChannelDeleter {
  void operator()(ares_channel channel) const noexcept {
    ::ares_destroy(channel);
  }
};

class ChannelPtr {
 public:
  ChannelPtr() = default;
  explicit ChannelPtr(ares_channel channel) : channel_{channel} {}

  auto Use() {
#if ARES_VERSION >= 0x011400
    // c-ares may lock internal mutexes while working with a channel,
    // ensure we will not switch threads while using it
    return compiler::ThreadLocalScope{channel_};
#else
    // But in old versions we must allow that
    return &channel_;
#endif
  }

 private:
  std::unique_ptr<std::remove_pointer_t<ares_channel>, ChannelDeleter> channel_;
};

struct AddrinfoDeleter {
  void operator()(struct ares_addrinfo* ai) const noexcept {
    ::ares_freeaddrinfo(ai);
  }
};
using AddrinfoPtr = std::unique_ptr<struct ares_addrinfo, AddrinfoDeleter>;

}  // namespace clients::dns::impl

USERVER_NAMESPACE_END
