#pragma once

#include <memory>

#include <boost/intrusive_ptr.hpp>

namespace utils {

template <class Target, class... Args>
boost::intrusive_ptr<Target> make_intrusive_ptr(Args&&... args) {
  auto ret = std::make_unique<Target>(std::forward<Args>(args)...);
  return ret.release();
}

}  // namespace utils
