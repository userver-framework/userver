#include <userver/formats/json/parser/int_parser.hpp>

#include <optional>

#include <boost/numeric/conversion/cast.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

namespace {

/* In JSON double with zero fractional part is a legal integer,
 * e.g. "3.0" is an integer 3 */
template <class Int>
std::optional<Int> DoubleToInt(double value) {
  auto i = std::round(value);
  if (std::fabs(i - value) > std::max(std::fabs(value), std::fabs(i)) *
                                 std::numeric_limits<double>::epsilon()) {
    return std::nullopt;
  }
  return boost::numeric_cast<Int>(i);
}

}  // namespace

void IntegralParser<std::int32_t>::Int64(std::int64_t i) {
  this->SetResult(boost::numeric_cast<std::int32_t>(i));
}

void IntegralParser<std::int32_t>::Uint64(std::uint64_t i) {
  this->SetResult(boost::numeric_cast<std::int32_t>(i));
}

void IntegralParser<std::int32_t>::Double(double value) {
  auto v = DoubleToInt<std::int32_t>(value);
  if (!v) this->Throw("double");
  this->SetResult(*std::move(v));
}

void IntegralParser<std::int64_t>::Int64(std::int64_t i) {
  this->SetResult(boost::numeric_cast<std::int64_t>(i));
}

void IntegralParser<std::int64_t>::Uint64(std::uint64_t i) {
  this->SetResult(boost::numeric_cast<std::int64_t>(i));
}

void IntegralParser<std::int64_t>::Double(double value) {
  auto v = DoubleToInt<std::int64_t>(value);
  if (!v) this->Throw("double");
  this->SetResult(*std::move(v));
}

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
