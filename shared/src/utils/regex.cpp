#include <utils/regex.hpp>

#include <boost/regex.hpp>

namespace utils {

struct regex::Impl {
  boost::regex r;

  explicit Impl(std::string_view pattern) : r(pattern.begin(), pattern.end()) {}
};

regex::regex(std::string_view pattern) : impl_(regex::Impl(pattern)) {}

regex::~regex() = default;

bool regex_match(std::string_view str, const regex& pattern) {
  return boost::regex_match(str.begin(), str.end(), pattern.impl_->r);
}

}  // namespace utils
