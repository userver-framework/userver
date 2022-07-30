#pragma once

#include <cstddef>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

struct ConnectionSettings final {
  size_t max_channels;

  bool secure;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END