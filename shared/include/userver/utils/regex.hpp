#pragma once

/// @file userver/utils/regex.hpp
/// @brief @copybrief utils::regex

#include <string>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_containers
///
/// @brief Small alias for boost::regex / std::regex without huge includes
class regex final {
 public:
  regex();
  explicit regex(std::string_view pattern);

  ~regex();

  regex(const regex&);
  regex(regex&&) noexcept;

  regex& operator=(const regex&);
  regex& operator=(regex&&) noexcept;

 private:
  struct Impl;
  utils::FastPimpl<Impl, 16, 8> impl_;

  friend bool regex_match(std::string_view str, const regex& pattern);
  friend bool regex_search(std::string_view str, const regex& pattern);
};

/// @brief Determines whether the regular expression matches the entire target
/// character sequence
bool regex_match(std::string_view str, const regex& pattern);

/// @brief Determines whether the regular expression matches anywhere in the
/// target character sequence
bool regex_search(std::string_view str, const regex& pattern);

}  // namespace utils

USERVER_NAMESPACE_END
