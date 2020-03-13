#pragma once

#include <chrono>

#include <boost/optional.hpp>

namespace redis {

boost::optional<std::chrono::microseconds> GetSocketPeerRtt(int fd);

}  // namespace redis
