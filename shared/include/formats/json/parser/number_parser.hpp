#pragma once

#include <formats/json/parser/typed_parser.hpp>

namespace formats::json::parser {

template <typename Number>
class NumberParser final : public formats::json::parser::TypedParser<Number> {
 public:
  using formats::json::parser::TypedParser<Number>::TypedParser;

 protected:
  void Int64(int64_t value) override { this->SetResult(value); }

  void Uint64(uint64_t value) override { this->SetResult(value); }

  void Double(double value) override { this->SetResult(double(value)); }

  std::string Expected() const override { return "number"; }
};

using DoubleParser = NumberParser<double>;

using FloatParser = NumberParser<float>;

}  // namespace formats::json::parser
