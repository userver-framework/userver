#pragma once

/// @file userver/utils/regex.hpp
/// @brief @copybrief utils::regex

#include <string>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

class smatch;

/// @ingroup userver_universal userver_containers
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

  bool operator==(const regex&) const;

  std::string str() const;

 private:
  struct Impl;
  utils::FastPimpl<Impl, 16, 8> impl_;

  friend class smatch;
  friend bool regex_match(std::string_view str, const regex& pattern);
  friend bool regex_match(const std::string& str, smatch& m,
                          const regex& pattern);
  friend bool regex_search(std::string_view str, const regex& pattern);
  friend bool regex_search(const std::string& str, smatch& m,
                           const regex& pattern);
  friend std::string regex_replace(std::string_view str, const regex& pattern,
                                   std::string_view repl);
};

/// @ingroup userver_universal userver_containers
///
/// @brief Small alias for boost::smatch / std::regex without huge includes
class smatch final {
 public:
  smatch();

  ~smatch();

  smatch(const smatch&);

  smatch& operator=(const smatch&);

  std::size_t size() const;
  std::string operator[](int sub) const;

 private:
  struct Impl;
  utils::FastPimpl<Impl, 80, 8> impl_;

  friend bool regex_match(std::string_view str, const regex& pattern);
  friend bool regex_match(const std::string& str, smatch& m,
                          const regex& pattern);
  friend bool regex_search(std::string_view str, const regex& pattern);
  friend bool regex_search(const std::string& str, smatch& m,
                           const regex& pattern);
  friend std::string regex_replace(std::string_view str, const regex& pattern,
                                   std::string_view repl);
};

// @brief Returns true if the specified regular expression matches
// the whole of the input. Fills in what matched in m.
bool regex_match(const std::string& str, smatch& m, const regex& pattern);

/// @brief Determines whether the regular expression matches the entire target
/// character sequence
bool regex_match(std::string_view str, const regex& pattern);

/// @brief Determines whether the regular expression matches anywhere in the
/// target character sequence. Fills in what matched in m
bool regex_search(const std::string& str, smatch& m, const regex& pattern);

/// @brief Determines whether the regular expression matches anywhere in the
/// target character sequence
bool regex_search(std::string_view str, const regex& pattern);

/// @brief Create a new string where all regular expression matches replaced
/// with repl
std::string regex_replace(std::string_view str, const regex& pattern,
                          std::string_view repl);

}  // namespace utils

USERVER_NAMESPACE_END
