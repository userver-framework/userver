#include <userver/utils/bytes_per_second.hpp>

#include <cstdlib>
#include <stdexcept>

#include <boost/algorithm/string/case_conv.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {
struct PrefixToMultiplier {
  const char* const prefix;
  long long multiplier;
};

constexpr PrefixToMultiplier kPrefixes[] = {
    {"b/s", 1},

    {"kbit/s", 125},
    {"kibit/s", 128},
    {"kb/s", 1000},
    {"kib/s", 1024},

    {"mbit/s", 125000},
    {"mibit/s", 131072},
    {"mb/s", 1000000},
    {"mib/s", 1048576},

    {"gbit/s", 125000000},
    {"gibit/s", 134217728},
    {"gb/s", 1000000000},
    {"gib/s", 1073741824},

    {"tbit/s", 125000000000},
    {"tibit/s", 137438953472},
    {"tb/s", 1000000000000},
    {"tib/s", 1099511627776},
};

}  // namespace

BytesPerSecond StringToBytesPerSecond(const std::string& data) {
  std::size_t parsed_size = 0;
  const auto new_to = std::stoll(data, &parsed_size, 10);

  if (new_to < 0) {
    throw std::logic_error("StringToBytesPerSecond: '" + data +
                           "' is negative");
  }

  std::string remained{data.c_str() + parsed_size};
  boost::algorithm::to_lower(remained, std::locale::classic());

  for (auto v : kPrefixes) {
    if (v.prefix == remained) {
      static constexpr auto kMax = std::numeric_limits<long long>::max();
      if (kMax / v.multiplier < new_to) {
        throw std::runtime_error(
            data + " can not be represented as B/s without precision loss");
      }

      return BytesPerSecond{new_to * v.multiplier};
    }
  }

  if (remained == "bit/s") {
    if (new_to / 8 * 8 != new_to) {
      throw std::runtime_error(std::to_string(new_to) +
                               "bit/s can not be represented as B/s");
    }

    return BytesPerSecond{new_to / 8};
  }

  throw std::logic_error("StringToBytesPerSecond: unknown format specifier '" +
                         std::string{remained} + "' in string '" + data + "'");
}

}  // namespace utils

USERVER_NAMESPACE_END
