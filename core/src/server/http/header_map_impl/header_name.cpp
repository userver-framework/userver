#include <server/http/header_map_impl/header_name.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http::header_map_impl {

namespace {

constexpr std::size_t kUppercaseToLowerMask = 32;
static_assert((static_cast<std::size_t>('A') | kUppercaseToLowerMask) == 'a');
static_assert((static_cast<std::size_t>('Z') | kUppercaseToLowerMask) == 'z');
static_assert((static_cast<std::size_t>('a') | kUppercaseToLowerMask) == 'a');
static_assert((static_cast<std::size_t>('z') | kUppercaseToLowerMask) == 'z');

}  // namespace

void ToLowerCaseInPlace(std::string& header_name) {
  // This loop is vectorized, no need to bother with SSE directly
  for (char& c : header_name) {
    const char mask = (c >= 'A' && c <= 'Z') ? kUppercaseToLowerMask : 0;
    c |= mask;
  }
}

std::string ToLowerCase(const std::string& header_name) {
  std::string header{header_name};

  ToLowerCaseInPlace(header);

  return header;
}

}  // namespace server::http::header_map_impl

USERVER_NAMESPACE_END
