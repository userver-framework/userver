#pragma once

/// @file userver/utils/make_intrusive_ptr.hpp
/// @brief @copybrief utils::make_intrusive_ptr

#include <memory>

#include <boost/intrusive_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief Factory function for boost::intrusive_ptr, like std::make_unique
template <class Target, class... Args>
boost::intrusive_ptr<Target> make_intrusive_ptr(Args&&... args) {
  auto ret = std::make_unique<Target>(std::forward<Args>(args)...);
  return ret.release();
}

}  // namespace utils

USERVER_NAMESPACE_END
