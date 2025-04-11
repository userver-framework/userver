#include <userver/formats/json/parser/int_parser.hpp>

#include <limits>
#include <optional>
#include <type_traits>

#include <userver/utils/numeric_cast.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

namespace {

/* In JSON double with zero fractional part is a legal integer,
 * e.g. "3.0" is an integer 3 */
template <typename Int>
std::optional<Int> DoubleToInt(double value) {
    auto i = std::round(value);
    if (std::abs(i - value) > std::max(std::abs(value), std::abs(i)) * std::numeric_limits<double>::epsilon()) {
        return std::nullopt;  // rounding
    }

    // Taken from the implementation of boost::numeric_cast<Int>(double).
    if (value <= static_cast<double>(std::numeric_limits<Int>::lowest()) - 1.0) {
        throw std::runtime_error("numeric conversion: negative overflow");
    }
    if (value >= static_cast<double>(std::numeric_limits<Int>::max()) + 1.0) {
        throw std::runtime_error("numeric conversion: positive overflow");
    }
    return value < 0.0 ? static_cast<Int>(std::ceil(value)) : static_cast<Int>(std::floor(value));
}

}  // namespace

void IntegralParser<std::int32_t>::Int64(std::int64_t i) { this->SetResult(utils::numeric_cast<std::int32_t>(i)); }

void IntegralParser<std::int32_t>::Uint64(std::uint64_t i) { this->SetResult(utils::numeric_cast<std::int32_t>(i)); }

void IntegralParser<std::int32_t>::Double(double value) {
    auto v = DoubleToInt<std::int32_t>(value);
    if (!v) this->Throw("double");
    this->SetResult(*std::move(v));
}

void IntegralParser<std::int64_t>::Int64(std::int64_t i) { this->SetResult(utils::numeric_cast<std::int64_t>(i)); }

void IntegralParser<std::int64_t>::Uint64(std::uint64_t i) { this->SetResult(utils::numeric_cast<std::int64_t>(i)); }

void IntegralParser<std::int64_t>::Double(double value) {
    auto v = DoubleToInt<std::int64_t>(value);
    if (!v) this->Throw("double");
    this->SetResult(*std::move(v));
}

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
