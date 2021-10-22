#pragma once

#include <memory>
#include <type_traits>

#include <ares.h>

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
using ChannelPtr =
    std::unique_ptr<std::remove_pointer_t<ares_channel>, ChannelDeleter>;

struct AddrinfoDeleter {
  void operator()(struct ares_addrinfo* ai) const noexcept {
    ::ares_freeaddrinfo(ai);
  }
};
using AddrinfoPtr = std::unique_ptr<struct ares_addrinfo, AddrinfoDeleter>;

}  // namespace clients::dns::impl

USERVER_NAMESPACE_END
