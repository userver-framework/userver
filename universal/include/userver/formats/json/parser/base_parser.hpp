#pragma once

/// @file userver/formats/json/parser/base_parser.hpp
/// @brief @copybrief formats::json::parser::BaseParser
/// @ingroup userver_universal

#include <userver/formats/json/parser/exception.hpp>
#include <userver/formats/json/parser/parser_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

/// @brief Base class for SAX parser.
///
class BaseParser {
public:
    virtual ~BaseParser();

    BaseParser() = default;
    BaseParser(BaseParser&&) = delete;
    BaseParser(const BaseParser&) = delete;
    BaseParser& operator=(const BaseParser&) = delete;
    BaseParser& operator=(BaseParser&&) = delete;

    virtual void Null();
    virtual void Bool(bool);
    virtual void Int64(int64_t);
    virtual void Uint64(uint64_t);
    virtual void Double(double);
    virtual void String(std::string_view);
    virtual void StartObject();
    virtual void Key(std::string_view key);
    virtual void EndObject();
    virtual void StartArray();
    virtual void EndArray();

    // Low-level variants of EndObject/EndArray
    virtual void EndObject(size_t /* members */) { EndObject(); }
    virtual void EndArray(size_t /* members */) { EndArray(); }

    void SetState(ParserState& state) { parser_state_ = &state; }

    virtual std::string GetPathItem() const = 0;

    std::string GetCurrentPath() const { return parser_state_->GetCurrentPath(); }

protected:
    [[noreturn]] void Throw(const std::string& found);

    virtual std::string Expected() const = 0;

    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    ParserState* parser_state_{nullptr};
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
