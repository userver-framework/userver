#include <utils/string_to_duration.hpp>

#include <cstdlib>
#include <stdexcept>
#include <utils/string_view.hpp>

namespace utils {

namespace {

template <class Rep, class Period>
std::chrono::milliseconds checked_convert(std::chrono::duration<Rep, Period> d,
                                          const std::string& data) {
  using namespace std::chrono;

  const duration<long double, milliseconds::period> extended_duration{d};
  if (extended_duration > milliseconds::max()) {
    throw std::overflow_error("StringToDuration overflow while representing '" +
                              data + "' as ms");
  } else if (extended_duration < milliseconds::min()) {
    throw std::underflow_error(
        "StringToDuration underflow while representing '" + data + "' as ms");
  }

  return duration_cast<milliseconds>(d);
}

}  // namespace

std::chrono::milliseconds StringToDuration(const std::string& data) {
  std::size_t parsed_size = 0;
  const auto new_to = std::stoll(data, &parsed_size, 10);

  if (new_to < 0) {
    throw std::logic_error("StringToDuration: '" + data + "' is negative");
  }

  const utils::string_view remained{data.c_str() + parsed_size};

  if (remained.empty() || remained == "s") {
    return checked_convert(std::chrono::seconds{new_to}, data);
  } else if (remained == "ms") {
    return checked_convert(std::chrono::milliseconds{new_to}, data);
  } else if (remained == "m") {
    return checked_convert(std::chrono::minutes{new_to}, data);
  } else if (remained == "h") {
    return checked_convert(std::chrono::hours{new_to}, data);
  } else if (remained == "d") {
    // `new_to * 24` may overflow, but it will be catched later while converting
    // hours to milliseconds.
    return checked_convert(std::chrono::hours{new_to * 24}, data);
  }

  throw std::logic_error("StringToDuration: unknown format specifier '" +
                         std::string{remained} + "' in string '" + data + "'");
}

}  // namespace utils
