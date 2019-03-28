#include <storages/postgres/io/boost_multiprecision.hpp>

#include <algorithm>
#include <boost/algorithm/string/trim.hpp>

#include <storages/postgres/io/integral_types.hpp>
#include <storages/postgres/io/pg_type_parsers.hpp>

namespace storages::postgres::io::detail {

namespace {

enum ValidSigns : std::uint16_t {
  kNumericPositive = 0x0000,
  kNumericNegative = 0x4000,
  kNumericNan = 0xc000
};

const Smallint kBinEncodingBase = 10000;

const std::uint16_t kDscaleMask = 0x3fff;

// Number of decimal digits in
const std::size_t kDigitWidth = 4;

void WriteDigit(std::string& res, std::uint16_t bin_dgt,
                bool truncate_leading_zeros) {
  char buff[]{'0', '0', '0', '0', '0', '0', '0', '0'};
  char* p = buff;
  std::ptrdiff_t significant_digits{0};
  while (bin_dgt) {
    auto dec_dgt = bin_dgt % 10;
    bin_dgt /= 10;
    *p++ += dec_dgt;
    ++significant_digits;
  }
  p = buff;
  auto e = p + (truncate_leading_zeros
                    ? (significant_digits ? significant_digits : 1)
                    : kDigitWidth);
  std::reverse_copy(p, e, std::back_inserter(res));
}

/// @brief Class for reading/writing binary numeric data from/to PostgreSQL
/// buffers.
///
/// A numeric value in the binary buffer is a binary coded decimal, 4 decimal
/// positions in two bytes. Those two bytes are further referred to as
/// binary digits or simply digits.
///
/// The numeric binary buffer looks like that:
// clang-format off
/// | Field   | Type            | Size in bytes |
/// | ndigits | uint16          | 4             |
/// | weight  | int16           | 4             |
/// | sign    | uint16          | 4             |
/// | dscale  | uint16          | 4             |
/// | digits  | vector<uint16>  | ndigits * 4   |
///
// clang-format on
/// * `ndigits` field contains the number of digits in the `digits` array.
/// * `weight` means the position of the first digit (in binary digit count)
///            before the decimal point, e.g. 0 means the first digit in the
///            `digits` array is the digit before the decimal point, 1 means
///            that the digit will go 1 binary digit position (4 decimal
///            positions) to the left and will have to be zero-padded in string
///            representation up to the decimal if there are no more binary
///            digits in the array. -1 means that the first digit is right after
///            the decimal point
/// * `sign`   sign of the number. Can have the following values:
///            0x0000 means positive number
///            0x4000 means negative number
///            0xc000 means NaN
/// * `dscale` is the number of decimal digits after the decimal point
/// * `digits` is the array of binary digits. Each binary digit represents 4
///            decimal positions
struct NumericData {
  using Digit = std::int16_t;
  using Digits = std::vector<Digit>;

  std::uint16_t ndigits;
  Smallint weight;
  std::uint16_t sign;
  std::uint16_t dscale;
  Digits digits;

  // Read from buffer
  void ReadBuffer(const FieldBuffer& fb);
  // Output to string
  std::string ToString() const;
};

void NumericData::ReadBuffer(const FieldBuffer& fb) {
  constexpr std::size_t smallint_size = sizeof(Smallint);
  std::size_t offset{0};

  ReadBinary(
      fb.GetSubBuffer(offset, smallint_size, BufferCategory::kPlainBuffer),
      ndigits);
  offset += smallint_size;

  ReadBinary(
      fb.GetSubBuffer(offset, smallint_size, BufferCategory::kPlainBuffer),
      weight);
  offset += smallint_size;

  ReadBinary(
      fb.GetSubBuffer(offset, smallint_size, BufferCategory::kPlainBuffer),
      sign);
  offset += smallint_size;

  if (!(sign == kNumericPositive || sign == kNumericNegative ||
        sign == kNumericNan)) {
    throw InvalidBinaryBuffer{"Unexpected numeric sign value"};
  }

  ReadBinary(
      fb.GetSubBuffer(offset, smallint_size, BufferCategory::kPlainBuffer),
      dscale);
  offset += smallint_size;
  if ((dscale & kDscaleMask) != dscale) {
    throw InvalidBinaryBuffer{"Invalid numeric dscale value"};
  }

  digits.reserve(ndigits);
  for (auto i = 0; i < ndigits; ++i) {
    Digit digit{0};
    ReadBinary(
        fb.GetSubBuffer(offset, smallint_size, BufferCategory::kPlainBuffer),
        digit);
    offset += smallint_size;
    if (digit < 0 || digit >= kBinEncodingBase) {
      throw InvalidBinaryBuffer{"Invalid binary digit value in numeric type"};
    }
    digits.push_back(digit);
  }
}

std::string NumericData::ToString() const {
  if (sign == kNumericNan) {
    return "NaN";
  }

  std::int32_t before_point = (weight + 1) * kDigitWidth;
  if (before_point <= 0) before_point = 1;

  std::string res;

  res.reserve(before_point + dscale + kDigitWidth + 1);

  if (sign == kNumericNegative) {
    res.push_back('-');
  }

  auto bd_pos = 0;
  if (weight < 0) {
    res.push_back('0');
    bd_pos = weight + 1;
  } else {
    for (bd_pos = 0; bd_pos <= weight; ++bd_pos) {
      auto dig = (bd_pos < ndigits) ? digits[bd_pos] : 0;
      WriteDigit(res, dig, bd_pos == 0);
    }
  }

  if (dscale > 0) {
    res.push_back('.');
    for (auto dec_pos = 0; dec_pos < dscale; ++bd_pos, dec_pos += kDigitWidth) {
      auto dig = (0 <= bd_pos && bd_pos < ndigits) ? digits[bd_pos] : 0;
      WriteDigit(res, dig, false);
    }
    // Truncate trailing zeros
    boost::trim_right_if(res, [](auto c) { return c == '0'; });
  }
  return res;
}

}  // namespace

std::string NumericBufferToString(const FieldBuffer& buffer) {
  NumericData nd;
  nd.ReadBuffer(buffer);
  return nd.ToString();
}

}  // namespace storages::postgres::io::detail
