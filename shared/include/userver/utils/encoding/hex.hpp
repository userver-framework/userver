#pragma once

/// @file userver/utils/encoding/hex.hpp
/// @brief Encoders, decoders and helpers for hexadecimal representations

#include <string>

USERVER_NAMESPACE_BEGIN

namespace utils::encoding {

/// @brief Converts number to hex character; number must be within range [0,16)
/// @throws out_of_bounds exception if \p num is out of range
char ToHexChar(int num);

/// @brief Calculate expected length of input after being hex encoded
/// @{

constexpr size_t LengthInHexForm(size_t size) noexcept { return size * 2; }

constexpr size_t LengthInHexForm(std::string_view data) noexcept {
  return LengthInHexForm(data.size());
}
/// @}

/// @brief Return upper limit on number of characters required
/// to unhex input of given size.
///
/// For example:
///  - FromHexUpperBound(1) = 0, because you can't really unhex one char,
///    it is only half byte. Where is the second half?
///  - FromHexUpperBound(2) = 1. Two chars will be converted into one byte
///  - FromHexUpperBound(5) = 2. First 4 chars will be unhexed into 2 bytes,
///    and there will be one left.
constexpr size_t FromHexUpperBound(size_t size) noexcept {
  // Although only even-sized input is valid, we have to support an odd
  // number is well. Luckily for us, size / 2 will get us correct value
  // anyway
  return size / 2;
}

/// @brief Converts input to hex and writes data to output \p out
/// @param input bytes to convert
/// @param out string to write data. out will be cleared
void ToHex(std::string_view input, std::string& out) noexcept;

/// @brief Allocates std::string, converts input and writes into said string
/// @param input range of input bytes
inline std::string ToHex(std::string_view data) noexcept {
  std::string result;
  ToHex(data, result);
  return result;
}

/// @brief Allocates std::string, converts input and writes into said string
/// @param data start of continuous range in memory
/// @param len size of that range
inline std::string ToHex(const void* encoded, size_t len) noexcept {
  const auto* chars = reinterpret_cast<const char*>(encoded);
  return ToHex(std::string_view{chars, len});
}

/// @brief Converts as much of input from hex as possible and writes data
/// into \p out.
///
/// To convert some range from hex, range
/// must have even number of elements and every element must be hex character.
/// To avoid throwing, algorithms consumes as much data as possible and returns
/// how much it was able to process
///
/// @param encoded input range to convert
/// @param out Result will be written into out. Previous value will be cleared.
/// @returns Number of characters successfully parsed.
size_t FromHex(std::string_view encoded, std::string& out) noexcept;

/// @brief This FromHex overload allocates string and calls FromHex; if data
/// is not fully a hex string, then it will be only partially processed.
inline std::string FromHex(std::string_view encoded) noexcept {
  std::string result;
  FromHex(encoded, result);
  return result;
}

/// Returns range that constitutes hex string - e.g. sub-view of encoded
/// that could  be fully interpreted as hex string.
/// Basically, if you have string_view \p a and c = GetHexPart(a), then
/// range FromHex(c) will be fully parsed.
/// @param encoded input array of bytes.
std::string_view GetHexPart(std::string_view encoded) noexcept;

/// @brief Checks that given range is fully a hex string. That is, if passed to
/// FromHex, it will be fully processed
bool IsHexData(std::string_view encoded) noexcept;

/// @brief iterprets uint64_t value as array of bytes and applies ToHex to it
inline std::string ToHexString(uint64_t value) {
  return ToHex(&value, sizeof(value));
}

}  // namespace utils::encoding

USERVER_NAMESPACE_END
