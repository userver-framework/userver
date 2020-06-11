#pragma once

#include <formats/json/parser/typed_parser.hpp>

namespace formats::json::parser {

class IntParser final : public TypedParser<int64_t> {
 public:
  using TypedParser::TypedParser;

 protected:
  void Int64(int64_t i) override;

  void Uint64(uint64_t i) override;

  std::string Expected() const override;
};

}  // namespace formats::json::parser
