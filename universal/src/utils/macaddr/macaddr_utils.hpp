#pragma once

#include <array>
#include <stdexcept>
#include <utility>

#include <userver/utils/encoding/hex.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::macaddr::detail {

/// @brief Get MAC-address octets from std::string_view
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

// template <size_t N>
// std::string MacaddrOctetsToString(const std::array<unsigned char, N>& octets)
// {

// }

}  // namespace utils::macaddr::detail

USERVER_NAMESPACE_END
