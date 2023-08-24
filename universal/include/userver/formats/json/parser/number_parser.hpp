#pragma once

#include <userver/formats/json/parser/typed_parser.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

template <typename Number>
class NumberParser final : public formats::json::parser::TypedParser<Number> {
 public:
  using formats::json::parser::TypedParser<Number>::TypedParser;

 protected:
  void Int64(int64_t value) override { this->SetResult(value); }

  void Uint64(uint64_t value) override { this->SetResult(value); }

  void Double(double value) override { this->SetResult(std::move(value)); }

  std::string Expected() const override { return "number"; }

  std::string GetPathItem() const override { return {}; }
};

using DoubleParser = NumberParser<double>;

using FloatParser = NumberParser<float>;

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
