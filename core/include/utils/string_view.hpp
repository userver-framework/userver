#pragma once

/// @file utils/string_view.hpp
/// @brief Alias for the available string_view implementation

#include <boost/version.hpp>
#include <string>

#ifdef __cpp_lib_string_view

#include <string_view>
namespace utils {
using string_view = std::string_view;
}  // namespace utils

#elif BOOST_VERSION < 106100

#include <boost/utility/string_ref.hpp>
namespace utils {
using string_view = boost::string_ref;
}  // namespace utils

#else

#include <boost/utility/string_view.hpp>
namespace utils {
using string_view = boost::string_view;
}  // namespace utils

#endif
