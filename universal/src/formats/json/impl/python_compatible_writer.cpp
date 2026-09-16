#include "python_compatible_writer.hpp"

#include <cmath>
#include <string>

#include <fmt/format.h>

#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::impl {
namespace {

constexpr std::string_view kHexDigits = "0123456789abcdef";

struct DecimalRepresentation final {
    std::string digits;
    int decimal_point{0};
    bool is_negative{false};
};

int ParseExponent(std::string_view exponent) {
    const bool is_negative = exponent.front() == '-';
    if (exponent.front() == '+' || exponent.front() == '-') {
        exponent.remove_prefix(1);
    }

    int result = 0;
    for (const char digit : exponent) {
        result = result * 10 + (digit - '0');
    }
    return is_negative ? -result : result;
}

DecimalRepresentation ParseShortest(std::string_view shortest) {
    DecimalRepresentation result;
    result.is_negative = shortest.front() == '-';
    if (result.is_negative) {
        shortest.remove_prefix(1);
    }

    const auto exponent_pos = shortest.find_first_of("eE");
    const auto mantissa = shortest.substr(0, exponent_pos);
    const int exponent = exponent_pos == std::string_view::npos ? 0 : ParseExponent(shortest.substr(exponent_pos + 1));
    const auto decimal_pos = mantissa.find('.');
    result.decimal_point =
        exponent + static_cast<int>(decimal_pos == std::string_view::npos ? mantissa.size() : decimal_pos);

    result.digits.reserve(mantissa.size());
    for (const char digit : mantissa) {
        if (digit != '.') {
            result.digits.push_back(digit);
        }
    }

    const auto first_digit = result.digits.find_first_not_of('0');
    if (first_digit == std::string::npos) {
        result.digits.clear();
        return result;
    }
    result.decimal_point -= static_cast<int>(first_digit);
    result.digits.erase(0, first_digit);
    while (result.digits.size() > 1 && result.digits.back() == '0') {
        result.digits.pop_back();
    }
    return result;
}

std::string FormatFixed(const DecimalRepresentation& decimal) {
    std::string result;
    result.reserve(32);
    if (decimal.is_negative) {
        result.push_back('-');
    }

    if (decimal.decimal_point <= 0) {
        result += "0.";
        result.append(static_cast<std::size_t>(-decimal.decimal_point), '0');
        result += decimal.digits;
    } else if (decimal.decimal_point >= static_cast<int>(decimal.digits.size())) {
        result += decimal.digits;
        result.append(static_cast<std::size_t>(decimal.decimal_point) - decimal.digits.size(), '0');
        result += ".0";
    } else {
        const auto point = static_cast<std::size_t>(decimal.decimal_point);
        result.append(decimal.digits, 0, point);
        result.push_back('.');
        result.append(decimal.digits, point, std::string::npos);
    }
    return result;
}

std::string FormatScientific(const DecimalRepresentation& decimal) {
    std::string result;
    result.reserve(32);
    if (decimal.is_negative) {
        result.push_back('-');
    }

    result.push_back(decimal.digits.front());
    if (decimal.digits.size() > 1) {
        result.push_back('.');
        result.append(decimal.digits, 1, std::string::npos);
    }

    const int exponent = decimal.decimal_point - 1;
    result.push_back('e');
    result.push_back(exponent < 0 ? '-' : '+');
    const int absolute_exponent = std::abs(exponent);
    if (absolute_exponent < 10) {
        result.push_back('0');
    }
    result += std::to_string(absolute_exponent);
    return result;
}

std::string FormatDouble(double value) {
    // Fmt provides correctly rounded shortest digits. Rewrite only the notation to match
    // Python's float repr thresholds, trailing `.0`, and exponent spelling.
    const auto decimal = ParseShortest(fmt::format("{}", value));
    if (decimal.digits.empty()) {
        return decimal.is_negative ? "-0.0" : "0.0";
    }
    if (decimal.decimal_point > -4 && decimal.decimal_point <= 16) {
        return FormatFixed(decimal);
    }
    return FormatScientific(decimal);
}

}  // namespace

PythonCompatibleWriter::PythonCompatibleWriter(rapidjson::StringBuffer& buffer)
    : Base(buffer)
{}

bool PythonCompatibleWriter::Double(double value) {
    Base::Prefix(rapidjson::kNumberType);
    return Base::EndValue(WriteDouble(value));
}

bool PythonCompatibleWriter::String(const char* str, rapidjson::SizeType length, [[maybe_unused]] bool copy) {
    Base::Prefix(rapidjson::kStringType);
    return Base::EndValue(WriteString(std::string_view{str, length}));
}

bool PythonCompatibleWriter::Key(const char* str, rapidjson::SizeType length, bool copy) {
    return String(str, length, copy);
}

bool PythonCompatibleWriter::WriteDouble(double value) {
    if (!std::isfinite(value)) {
        return false;
    }

    const auto formatted = FormatDouble(value);
    for (const char ch : formatted) {
        Base::os_->Put(ch);
    }
    return true;
}

bool PythonCompatibleWriter::WriteString(std::string_view str) {
    if (!utils::text::IsUtf8(str)) {
        return false;
    }

    Base::os_->Put('"');
    for (const char ch : str) {
        const auto byte = static_cast<unsigned char>(ch);
        switch (byte) {
            case '"':
                Base::os_->Put('\\');
                Base::os_->Put('"');
                break;
            case '\\':
                Base::os_->Put('\\');
                Base::os_->Put('\\');
                break;
            case '\b':
                Base::os_->Put('\\');
                Base::os_->Put('b');
                break;
            case '\t':
                Base::os_->Put('\\');
                Base::os_->Put('t');
                break;
            case '\n':
                Base::os_->Put('\\');
                Base::os_->Put('n');
                break;
            case '\f':
                Base::os_->Put('\\');
                Base::os_->Put('f');
                break;
            case '\r':
                Base::os_->Put('\\');
                Base::os_->Put('r');
                break;
            default:
                if (byte < 0x20) {
                    Base::os_->Put('\\');
                    Base::os_->Put('u');
                    Base::os_->Put('0');
                    Base::os_->Put('0');
                    Base::os_->Put(kHexDigits[byte >> 4]);
                    Base::os_->Put(kHexDigits[byte & 0x0f]);
                } else {
                    Base::os_->Put(ch);
                }
        }
    }
    Base::os_->Put('"');
    return true;
}

}  // namespace formats::json::impl

USERVER_NAMESPACE_END
