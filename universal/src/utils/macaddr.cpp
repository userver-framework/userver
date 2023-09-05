#include <userver/utils/macaddr.hpp>

#include <fmt/printf.h>

#include <array>
#include <stdexcept>
#include <utility>

#include <userver/utils/encoding/hex.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

template <size_t N>
auto MacaddrOctetsFromString(const std::string& str) {
  std::array<unsigned char, N> octets = {0};
  size_t octets_counter = 0;
  size_t index = 0;
  char spacer = '\0';

  const auto throw_exception = []() {
    throw std::invalid_argument(
        "Error while converting MAC-address from string");
  };

  const auto hex2_to_uchar = [&str, &octets, throw_exception](
                                 size_t index, size_t octets_counter) {
    std::string out;
    auto result =
        USERVER_NAMESPACE::utils::encoding::FromHex(str.substr(index, 2), out);
    if (result < 2) {
      throw_exception();
    }
    octets[octets_counter - 1] = out.at(0);
  };

  while (index < str.size()) {
    octets_counter++;
    if (octets_counter >= 1 && octets_counter <= octets.size()) {
      hex2_to_uchar(index, octets_counter);
    } else {
      throw_exception();
    }
    index += 2;
    const char symbol = str[index];
    if (symbol == ':' || symbol == '-' || symbol == '.') {
      if (spacer == '\0') {
        spacer = symbol;
      } else if (spacer != symbol) {
        throw_exception();
      }
      index++;
    }
  }
  return std::make_pair(octets, octets_counter);
}

}  // namespace

Macaddr MacaddrFromString(const std::string& str) {
  const auto [octets, octets_counter] =
      MacaddrOctetsFromString<Macaddr::OctetsType().size()>(str);
  if (octets_counter != octets.size()) {
    throw std::invalid_argument(std::to_string(octets_counter));
  }
  return Macaddr(octets);
}

std::string MacaddrToString(Macaddr macaddr) {
  const auto octets = macaddr.GetOctets();
  return fmt::sprintf("%02x:%02x:%02x:%02x:%02x:%02x", octets[0], octets[1],
                      octets[2], octets[3], octets[4], octets[5]);
}

Macaddr8 Macaddr8FromString(const std::string& str) {
  auto [octets, octets_counter] =
      MacaddrOctetsFromString<Macaddr8::OctetsType().size()>(str);
  if (octets_counter == 6) {
    octets[7] = octets[5];
    octets[6] = octets[4];
    octets[5] = octets[3];
    octets[3] = 0xFF;
    octets[4] = 0xFE;
  } else if (octets_counter != 8) {
    throw std::invalid_argument(
        "Error while converting MAC-address from string");
  }
  return Macaddr8(octets);
}

std::string Macaddr8ToString(Macaddr8 macaddr) {
  const auto octets = macaddr.GetOctets();
  return fmt::sprintf("%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", octets[0],
                      octets[1], octets[2], octets[3], octets[4], octets[5],
                      octets[6], octets[7]);
}

}  // namespace utils

USERVER_NAMESPACE_END
