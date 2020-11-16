#include <utils/regex.hpp>

#include <boost/regex.hpp>

namespace utils {

struct regex::Impl {
  boost::regex r;

  Impl() = default;
  explicit Impl(std::string_view pattern) : r(pattern.begin(), pattern.end()) {}
};

regex::regex() : impl_() {}

regex::regex(std::string_view pattern) : impl_(regex::Impl(pattern)) {}

regex::~regex() = default;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
regex::regex(const regex&) = default;

regex::regex(regex&& r) noexcept : impl_() { impl_->r.swap(r.impl_->r); }

regex& regex::operator=(const regex&) = default;

regex& regex::operator=(regex&& r) noexcept {
  impl_->r.swap(r.impl_->r);
  return *this;
}

bool regex_match(std::string_view str, const regex& pattern) {
  return boost::regex_match(str.begin(), str.end(), pattern.impl_->r);
}

bool regex_search(std::string_view str, const regex& pattern) {
  return boost::regex_search(str.begin(), str.end(), pattern.impl_->r);
}

}  // namespace utils
