#include <storages/postgres/io/boost_multiprecision.hpp>

#include <algorithm>
#include <boost/algorithm/string/trim.hpp>

#include <storages/postgres/io/field_buffer.hpp>
#include <storages/postgres/io/integral_types.hpp>
#include <storages/postgres/io/pg_type_parsers.hpp>
#include <storages/postgres/io/user_types.hpp>

#include <utils/str_icase.hpp>
#include <utils/string_view.hpp>

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
const int kDigitWidth = 4;

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

// Get a digit from the string representation, checking index range
// and if the char is a digit
std::int16_t GetBinDigit(::utils::string_view str, int idx) {
  if (idx < 0 || static_cast<std::int64_t>(str.size()) <= idx) {
    return 0;
  }
  auto c = str[idx];
  if (!std::isdigit(c)) {
    throw InvalidInputFormat{
        "String representation for a numeric contains a non-digit"};
  }
  return c - '0';
}

std::int16_t GetPaddedDigit(::utils::string_view str, int idx) {
  std::int16_t val = 0;
  for (auto i = 0U; i < kDigitWidth; ++i) {
    val *= 10;
    val += GetBinDigit(str, idx + i);
  }
  return val;
}

void ConvertDecimalToBinary(::utils::string_view dec_digits, int left_padding,
                            std::vector<std::int16_t>& target) {
  for (auto dec_pos = left_padding;
       dec_pos < static_cast<std::int32_t>(dec_digits.size());
       dec_pos += kDigitWidth) {
    target.push_back(GetPaddedDigit(dec_digits, dec_pos));
  }
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

  std::uint16_t ndigits = 0;
  Smallint weight = 0;
  std::uint16_t sign = kNumericPositive;
  std::uint16_t dscale = 0;
  Digits digits;

  // Read from buffer
  void ReadBuffer(FieldBuffer fb);
  // Get binary representation
  std::string GetBuffer() const;
  // Parse string
  void Parse(const std::string&);
  // Output to string
  std::string ToString() const;
};

void NumericData::ReadBuffer(FieldBuffer fb) {
  fb.Read(ndigits);
  fb.Read(weight);
  fb.Read(sign);

  if (!(sign == kNumericPositive || sign == kNumericNegative ||
        sign == kNumericNan)) {
    throw InvalidBinaryBuffer{"Unexpected numeric sign value"};
  }

  fb.Read(dscale);
  if ((dscale & kDscaleMask) != dscale) {
    throw InvalidBinaryBuffer{"Invalid numeric dscale value"};
  }

  digits.reserve(ndigits);
  for (auto i = 0; i < ndigits; ++i) {
    Digit digit{0};
    fb.Read(digit);
    if (digit < 0 || digit >= kBinEncodingBase) {
      throw InvalidBinaryBuffer{"Invalid binary digit value in numeric type"};
    }
    digits.push_back(digit);
  }
}

std::string NumericData::GetBuffer() const {
  static const UserTypes types;
  std::string buff;
  WriteBinary(types, buff, ndigits);
  WriteBinary(types, buff, weight);
  WriteBinary(types, buff, sign);
  WriteBinary(types, buff, dscale);
  for (auto dgt : digits) {
    WriteBinary(types, buff, dgt);
  }
  return buff;
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

void NumericData::Parse(const std::string& str) {
  if (str.empty()) {
    throw InvalidInputFormat{"Empty numeric string representation"};
  }

  // cpp_dec_float outputs NaN in lower case
  if (::utils::StrIcaseEqual{}(str, "nan")) {
    sign = kNumericNan;
    return;
  }

  // Deal with signs
  auto start_pos{0};
  switch (str[0]) {
    case '+':
      ++start_pos;
      break;
    case '-':
      sign = kNumericNegative;
      ++start_pos;
      break;
    default:
      break;
  }
  // Skip leading zeros
  auto non_z = str.find_first_not_of('0', start_pos);
  if (non_z == std::string::npos) {
    // String of zeros
    return;
  } else {
    start_pos = non_z;
  }
  // Now split to parts
  auto dec_point_pos = str.find('.', start_pos);
  if (str.find_first_of("eE", start_pos) != std::string::npos) {
    throw InvalidInputFormat{
        "Scientific string representation for numerics is not supported"};
  }
  auto int_part_end = str.size();
  if (dec_point_pos != std::string::npos) {
    int_part_end = dec_point_pos;
  }

  ::utils::string_view integral_part{str.data() + start_pos,
                                     int_part_end - start_pos};
  // TODO Check if size is reasonable
  std::int32_t dec_weight = integral_part.size() - 1;
  std::uint16_t dec_scale{0};

  ::utils::string_view fractional_part{};
  if (dec_point_pos != std::string::npos) {
    auto frac_part_begin = dec_point_pos + 1;
    auto frac_part_end = str.size();
    if (frac_part_begin == frac_part_end) {
      // No symbols after decimal point
      throw InvalidInputFormat{
          "Invalid numeric string format: no symbols after decimal point"};
    }
    if (integral_part.empty()) {
      // Skip leading zeros if there is nothing before decimal points
      non_z = str.find_first_not_of('0', frac_part_begin);
      if (non_z == std::string::npos) {
        // Fractional part contains only zeros and integral part is empty
        return;
      } else {
        frac_part_begin = non_z;
      }
      dec_weight = -(frac_part_begin - dec_point_pos);
    }
    fractional_part = {str.data() + frac_part_begin,
                       frac_part_end - frac_part_begin};
    // Cut trailing zeros
    non_z = fractional_part.find_last_not_of('0');
    if (non_z != std::string::npos) {
      dec_scale = non_z + (frac_part_begin - dec_point_pos);
      fractional_part = {str.data() + frac_part_begin, non_z + 1};
    } else if (integral_part.empty()) {
      // Fractional part contains only zeros and integral part is empty
      return;
    } else {
      // Fractional part contains only zeros
      fractional_part = {};
    }
  }

  // Convert to binary digits
  digits.reserve((integral_part.size() + fractional_part.size()) / kDigitWidth +
                 1);
  if (dec_weight >= 0) {
    weight = dec_weight / kDigitWidth;
  } else {
    weight = -((-dec_weight - 1) / kDigitWidth + 1);
  }
  // Padding offset is a coordinate relative the first digit in the sequence
  // (0 - the first digit, -1 - the digit before first). Negative indexes are
  // used for zero-padding
  std::int32_t padding_offset = dec_weight - (weight + 1) * kDigitWidth + 1;
  if (dec_weight >= 0) {
    ConvertDecimalToBinary(integral_part, padding_offset, digits);
    padding_offset = 0;
  }
  ConvertDecimalToBinary(fractional_part, padding_offset, digits);
  dscale = dec_scale;
  // Trim trailing zeros
  boost::trim_right_if(digits, [](auto c) { return c == 0; });
  ndigits = digits.size();
}

}  // namespace

std::string NumericBufferToString(const FieldBuffer& buffer) {
  NumericData nd;
  nd.ReadBuffer(buffer);
  return nd.ToString();
}

std::string StringToNumericBuffer(const std::string& str_rep) {
  NumericData nd;
  nd.Parse(str_rep);
  return nd.GetBuffer();
}

}  // namespace storages::postgres::io::detail
