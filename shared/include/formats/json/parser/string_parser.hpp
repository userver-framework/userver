#pragma once

#include <formats/json/parser/typed_parser.hpp>

namespace formats::json::parser {

class StringParser final : public TypedParser<std::string> {
 public:
  using TypedParser::TypedParser;

 protected:
  void String(std::string_view sw) override;

  std::string GetPathItem() const override { return {}; }

  std::string Expected() const override;
};

}  // namespace formats::json::parser
