#pragma once

#include <boost/numeric/conversion/cast.hpp>

#include <formats/json/parser/typed_parser.hpp>

namespace formats::json::parser {

template <typename T = int>
class IntegralParser final : public TypedParser<T> {
 public:
  using TypedParser<T>::TypedParser;

 protected:
  void Int64(int64_t i) override { this->SetResult(boost::numeric_cast<T>(i)); }

  void Uint64(uint64_t i) override {
    this->SetResult(boost::numeric_cast<T>(i));
  }

  /* In JSON double with zero fractional part is a legal integer,
   * e.g. "3.0" is an integer 3 */
  void Double(double value) override {
    auto i = std::round(value);
    if (std::fabs(i - value) > value * std::numeric_limits<double>::epsilon()) {
      this->Throw("double");
    }
    this->SetResult(boost::numeric_cast<T>(i));
  }

  std::string GetPathItem() const override { return {}; }

  std::string Expected() const override { return "integer"; }
};

using IntParser = IntegralParser<int32_t>;
using Int32Parser = IntegralParser<int32_t>;
using Int64Parser = IntegralParser<int64_t>;

}  // namespace formats::json::parser
