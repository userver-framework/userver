#include <userver/utils/regex.hpp>

#include <iterator>

#include <boost/regex.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

struct regex::Impl {
  boost::regex r;

  Impl() = default;
  explicit Impl(std::string_view pattern) : r(pattern.begin(), pattern.end()) {}
};

regex::regex() = default;

regex::regex(std::string_view pattern) : impl_(regex::Impl(pattern)) {}

regex::~regex() = default;

regex::regex(const regex&) = default;

regex::regex(regex&& r) noexcept { impl_->r.swap(r.impl_->r); }

regex& regex::operator=(const regex&) = default;

regex& regex::operator=(regex&& r) noexcept {
  impl_->r.swap(r.impl_->r);
  return *this;
}

bool regex::operator==(const regex& other) const {
  return impl_->r == other.impl_->r;
}

std::string regex::str() const { return impl_->r.str(); }

////////////////////////////////////////////////////////////////

struct match_results::Impl {
  boost::cmatch m;

  Impl() = default;
};

match_results::match_results() = default;

match_results::~match_results() = default;

match_results::match_results(const match_results&) = default;

match_results& match_results::operator=(const match_results&) = default;

std::size_t match_results::size() const { return impl_->m.size(); }

std::string_view match_results::operator[](int sub) const {
  auto substr = impl_->m[sub];
  return {&*substr.begin(), static_cast<std::size_t>(substr.length())};
}

////////////////////////////////////////////////////////////////

bool regex_match(std::string_view str, const regex& pattern) {
  return boost::regex_match(str.begin(), str.end(), pattern.impl_->r);
}

bool regex_match(std::string_view str, match_results& m, const regex& pattern) {
  return boost::regex_match(str.begin(), str.end(), m.impl_->m,
                            pattern.impl_->r);
}

bool regex_search(std::string_view str, const regex& pattern) {
  return boost::regex_search(str.begin(), str.end(), pattern.impl_->r);
}

bool regex_search(std::string_view str, match_results& m,
                  const regex& pattern) {
  return boost::regex_search(str.begin(), str.end(), m.impl_->m,
                             pattern.impl_->r);
}

std::string regex_replace(std::string_view str, const regex& pattern,
                          std::string_view repl) {
  std::string res;
  res.reserve(str.size() + str.size() / 4);

  boost::regex_replace(std::back_inserter(res), str.begin(), str.end(),
                       pattern.impl_->r, repl);

  return res;
}

}  // namespace utils

USERVER_NAMESPACE_END
