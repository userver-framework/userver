#pragma once

#include <userver/formats/json/parser/typed_parser.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

class BoolParser final : public formats::json::parser::TypedParser<bool> {
 public:
  using formats::json::parser::TypedParser<bool>::TypedParser;

 protected:
  void Bool(bool b) override;

  std::string GetPathItem() const override;

  std::string Expected() const override;
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
