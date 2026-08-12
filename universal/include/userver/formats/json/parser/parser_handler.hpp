#pragma once

/// @file userver/formats/json/parser/parser_handler.hpp
/// @brief @copybrief formats::json::parser::ParserHandler
/// @ingroup userver_universal

#include <userver/formats/json/parser/base_parser.hpp>
#include <userver/formats/json/parser/parser_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

/// @brief Dispatches JSON SAX events to a parser stack.
class ParserHandler final {
public:
    ParserHandler(BaseParser& parser);

    ParserHandler(ParserState& parser_state);

    bool Null();
    bool Bool(bool b);
    bool Int(int64_t i);
    bool Uint(uint64_t u);
    bool Int64(int64_t i);
    bool Uint64(uint64_t u);
    bool Double(double d);
    bool StartObject();
    bool EndObject(size_t);
    bool StartArray();
    bool EndArray(size_t);

    bool Key(const char* c, size_t size, bool);
    bool String(const char* c, size_t size, bool);
    bool RawNumber(const char*, size_t, bool) noexcept;

private:
    BaseParser& parser_;
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
