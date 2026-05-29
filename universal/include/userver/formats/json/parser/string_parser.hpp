#pragma once

/// @file userver/formats/json/parser/string_parser.hpp
/// @brief @copybrief formats::json::parser::StringParser
/// @ingroup userver_universal

#include <userver/formats/json/parser/typed_parser.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

/// @brief SAX parser for JSON string values.
class StringParser final : public TypedParser<std::string> {
public:
    using TypedParser::TypedParser;

protected:
    void String(std::string_view sw) override;

    std::string GetPathItem() const override { return {}; }

    std::string Expected() const override;
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
