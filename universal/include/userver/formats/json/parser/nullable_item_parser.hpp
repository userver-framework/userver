#pragma once

/// @file userver/formats/json/parser/nullable_item_parser.hpp
/// @brief @copybrief formats::json::parser::NullableItemParser
/// @ingroup userver_universal

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include <userver/formats/json/parser/typed_parser.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

/// @brief SAX parser for nullable items of containers, e.g. for `std::optional<T>` items of
/// formats::json::parser::ArrayParser.
///
/// Sends a value-initialized `Item` for JSON `null`. Delegates any other value to `ItemParser` and sends its result
/// converted to `Item`.
template <typename Item, typename ItemParser>
class NullableItemParser final : public TypedParser<Item>, public Subscriber<typename ItemParser::ResultType> {
public:
    explicit NullableItemParser(ItemParser& item_parser)
        : item_parser_(item_parser)
    {
        item_parser_.Subscribe(*this);
    }

protected:
    void Null() override { this->SetResult(Item{}); }

    void Bool(bool value) override { PushItemParser().Bool(value); }

    void Int64(std::int64_t value) override { PushItemParser().Int64(value); }

    void Uint64(std::uint64_t value) override { PushItemParser().Uint64(value); }

    void Double(double value) override { PushItemParser().Double(value); }

    void String(std::string_view value) override { PushItemParser().String(value); }

    void StartObject() override { PushItemParser().StartObject(); }

    void StartArray() override { PushItemParser().StartArray(); }

    std::string GetPathItem() const override { return {}; }

    std::string Expected() const override { return "null or item"; }

private:
    void OnSend(typename ItemParser::ResultType&& value) override { this->SetResult(Item(std::move(value))); }

    BaseParser& PushItemParser() {
        item_parser_.Reset();
        BaseParser& parser = item_parser_.GetParser();
        this->parser_state_->PushParser(parser);
        return parser;
    }

    ItemParser& item_parser_;
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
