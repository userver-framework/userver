#include <userver/storages/postgres/io/numeric_data.hpp>

#include <algorithm>
#include <array>
#include <string_view>

#include <boost/algorithm/string/trim.hpp>

#include <storages/postgres/io/pg_type_parsers.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/field_buffer.hpp>
#include <userver/storages/postgres/io/integral_types.hpp>
#include <userver/storages/postgres/io/user_types.hpp>

#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io::detail {

namespace {

enum ValidSigns : std::uint16_t {
  kNumericPositive = 0x0000,
  kNumericNegative = 0x4000,
  kNumericNan = 0xc000
};

const Smallint kBinEncodingBase = 10000;
const std::int64_t kPowersOfTen[]{1,
                                  10,
                                  100,
                                  1'000,
                                  10'000,
                                  100'000,
                                  1'000'000,
                                  10'000'000,
                                  100'000'000,
                                  1'000'000'000,
                                  10'000'000'000,
                                  100'000'000'000,
                                  1'000'000'000'000,
                                  10'000'000'000'000,
                                  100'000'000'000'000,
                                  1'000'000'000'000'000,
                                  10'000'000'000'000'000,
                                  100'000'000'000'000'000,
                                  1'000'000'000'000'000'000};

const auto kMaxPowerOfTen = sizeof(kPowersOfTen) / sizeof(kPowersOfTen[0]) - 1;

const std::uint16_t kDscaleMask = 0x3fff;

// Number of decimal digits in
const int kDigitWidth = 4;

void WriteDigit(std::string& res, std::uint16_t bin_dgt,
                bool truncate_leading_zeros) {
  std::array<char, 8> buffer{'0', '0', '0', '0', '0', '0', '0', '0'};

  char* p = buffer.data();
  std::ptrdiff_t significant_digits{0};
  while (bin_dgt) {
    auto dec_dgt = bin_dgt % 10;
    bin_dgt /= 10;
    *p++ += dec_dgt;
    ++significant_digits;
  }
  p = buffer.data();
  auto* e = p + (truncate_leading_zeros
                     ? (significant_digits ? significant_digits : 1)
                     : kDigitWidth);
  std::reverse_copy(p, e, std::back_inserter(res));
}

// Get a digit from the string representation, checking index range
// and if the char is a digit
std::int16_t GetBinDigit(std::string_view str, int idx) {
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

std::int16_t GetPaddedDigit(std::string_view str, int idx) {
  std::int16_t val = 0;
  for (int i = 0; i < kDigitWidth; ++i) {
    val *= 10;
    val += GetBinDigit(str, idx + i);
  }
  return val;
}

void ConvertDecimalToBinary(std::string_view dec_digits, int left_padding,
                            std::vector<std::int16_t>& target) {
  for (auto dec_pos = left_padding;
       dec_pos < static_cast<std::int32_t>(dec_digits.size());
       dec_pos += kDigitWidth) {
    target.push_back(GetPaddedDigit(dec_digits, dec_pos));
  }
}

/// Calculate number of decimal digits
int Log10(std::int64_t number) {
  for (auto p = 0U; p < kMaxPowerOfTen; ++p) {
    if (number < kPowersOfTen[p]) {
      return p;
    }
  }
  return kMaxPowerOfTen;
}

void IntegralToBinary(std::int64_t integral_part, Smallint digits,
                      std::vector<std::int16_t>& target) {
  // Left pad
  if (digits % kDigitWidth) {
    digits += kDigitWidth - digits % kDigitWidth;
  }
  while (digits > 0) {
    digits -= kDigitWidth;
    target.push_back(integral_part / kPowersOfTen[digits]);
    integral_part %= kPowersOfTen[digits];
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
  [[nodiscard]] std::string GetBuffer() const;
  // Parse string
  void Parse(const std::string&);
  // Output to string
  [[nodiscard]] std::string ToString() const;
  // Create buffer representation from an int64 value representation
  void FromInt64(IntegralRepresentation);
  // Output to int64 value
  [[nodiscard]] IntegralRepresentation ToInt64() const;
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
  io::WriteBuffer(types, buff, ndigits);
  io::WriteBuffer(types, buff, weight);
  io::WriteBuffer(types, buff, sign);
  io::WriteBuffer(types, buff, dscale);
  for (auto dgt : digits) {
    io::WriteBuffer(types, buff, dgt);
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

  auto bin_digit_pos = 0;
  if (weight < 0) {
    res.push_back('0');
    bin_digit_pos = weight + 1;
  } else {
    for (bin_digit_pos = 0; bin_digit_pos <= weight; ++bin_digit_pos) {
      auto dig = (bin_digit_pos < ndigits) ? digits[bin_digit_pos] : 0;
      WriteDigit(res, dig, bin_digit_pos == 0);
    }
  }

  if (dscale > 0) {
    res.push_back('.');
    for (auto dec_pos = 0; dec_pos < dscale;
         ++bin_digit_pos, dec_pos += kDigitWidth) {
      auto dig = (0 <= bin_digit_pos && bin_digit_pos < ndigits)
                     ? digits[bin_digit_pos]
                     : 0;
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
  if (USERVER_NAMESPACE::utils::StrIcaseEqual{}(str, "nan")) {
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

  std::string_view integral_part{str.data() + start_pos,
                                 int_part_end - start_pos};
  // TODO Check if size is reasonable
  std::int32_t dec_weight = integral_part.size() - 1;
  std::uint16_t dec_scale{0};

  std::string_view fractional_part{};
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

IntegralRepresentation NumericData::ToInt64() const {
  const std::uint64_t int64_t_max = std::numeric_limits<std::int64_t>::max();
  if (sign == kNumericNan) {
    throw ValueIsNaN{
        "PostgreSQL buffer for decimal/numeric contains a NaN value"};
  }
  IntegralRepresentation rep{0, dscale};
  auto bin_dig_pos = 0;
  if (weight < 0) {
    bin_dig_pos = weight + 1;
  } else {
    for (bin_dig_pos = 0; bin_dig_pos <= weight; ++bin_dig_pos) {
      auto dig = (bin_dig_pos < ndigits) ? digits[bin_dig_pos] : 0;
      std::uint64_t new_val = (rep.value * kBinEncodingBase) + dig;
      if (rep.value > static_cast<std::int64_t>(new_val) ||
          new_val > int64_t_max) {
        throw NumericOverflow{
            "PosrgreSQL buffer contains a value that is too big to fit into "
            "int64 + '" +
            ToString() + "'"};
      }
      rep.value = static_cast<std::int64_t>(new_val);
    }
  }

  if (dscale > 0) {
    for (auto decimal_pos = 0; decimal_pos < dscale;
         ++bin_dig_pos, decimal_pos += kDigitWidth) {
      auto dig =
          (0 <= bin_dig_pos && bin_dig_pos < ndigits) ? digits[bin_dig_pos] : 0;
      // Truncation of trailing zeros happens only on the last iteration and is
      // needed to avoid a possible overflow
      auto truncate_count = kDigitWidth + decimal_pos - dscale;
      auto power = kBinEncodingBase;
      if (truncate_count > 0) {
        power = kBinEncodingBase / kPowersOfTen[truncate_count];
        dig /= kPowersOfTen[truncate_count];
      }
      std::uint64_t new_val = (rep.value * power) + dig;
      if (rep.value > static_cast<std::int64_t>(new_val) ||
          new_val > int64_t_max) {
        throw NumericOverflow{
            "PosrgreSQL buffer contains a value that is too big to fit into "
            "int64 + '" +
            ToString() + "'"};
      }
      rep.value = static_cast<std::int64_t>(new_val);
    }
  }
  if (sign == kNumericNegative) {
    rep.value *= -1;
  }
  return rep;
}

void NumericData::FromInt64(IntegralRepresentation rep) {
  if (rep.fractional_digit_count < 0 ||
      static_cast<std::int64_t>(kMaxPowerOfTen) < rep.fractional_digit_count) {
    throw InvalidRepresentation{
        "Number of digits after decimal point is invalid " +
        std::to_string(rep.fractional_digit_count)};
  }
  if (rep.value == 0) return;

  sign = rep.value < 0 ? kNumericNegative : kNumericPositive;
  std::uint64_t abs_value = std::abs(rep.value);
  std::int64_t integral_part =
      abs_value / kPowersOfTen[rep.fractional_digit_count];
  std::int64_t fractional_part =
      abs_value % kPowersOfTen[rep.fractional_digit_count];

  auto integral_digits = Log10(integral_part);
  auto fractional_digits = rep.fractional_digit_count;

  // Remove trailing zeros, but keeping the digit number divisible by binary
  // digit width
  while (fractional_part && fractional_part % 10 == 0 &&
         fractional_digits % kDigitWidth != 0) {
    fractional_part /= 10;
    --fractional_digits;
  }
  // Here are the actual fractional digits
  dscale = fractional_digits;
  // Right pad fractional part to 4 digits boundary
  if (fractional_digits % kDigitWidth) {
    auto right_pad = kDigitWidth - (fractional_digits % kDigitWidth);
    fractional_digits += right_pad;
    fractional_part *= kPowersOfTen[right_pad];
  }

  std::int32_t dec_weight = integral_digits - 1;
  if (integral_part == 0) {
    dec_weight = Log10(fractional_part) - fractional_digits - 1;
  }

  // Convert to binary digits
  digits.reserve((integral_digits + fractional_digits) / kDigitWidth + 1);
  if (dec_weight >= 0) {
    weight = dec_weight / kDigitWidth;
  } else {
    weight = -((-dec_weight - 1) / kDigitWidth + 1);
  }
  if (weight >= 0) {
    IntegralToBinary(integral_part, integral_digits, digits);
  }
  IntegralToBinary(fractional_part, fractional_digits, digits);

  // Trim zero binary digits
  boost::trim_if(digits, [](auto c) { return c == 0; });
  ndigits = digits.size();
}

}  // namespace

std::string NumericBufferToString(const FieldBuffer& buffer) {
  NumericData num_data;
  num_data.ReadBuffer(buffer);
  return num_data.ToString();
}

std::string StringToNumericBuffer(const std::string& str_rep) {
  NumericData num_data;
  num_data.Parse(str_rep);
  return num_data.GetBuffer();
}

IntegralRepresentation NumericBufferToInt64(const FieldBuffer& buffer) {
  NumericData num_data;
  num_data.ReadBuffer(buffer);
  return num_data.ToInt64();
}

std::string Int64ToNumericBuffer(const IntegralRepresentation& rep) {
  NumericData num_data;
  num_data.FromInt64(rep);
  return num_data.GetBuffer();
}

}  // namespace storages::postgres::io::detail

USERVER_NAMESPACE_END
