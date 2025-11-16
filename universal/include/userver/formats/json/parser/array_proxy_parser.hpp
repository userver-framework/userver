#pragma once

#include <memory>
#include <userver/formats/json/parser/array_parser.hpp>
#include <userver/formats/json/parser/base_parser.hpp>
#include <userver/formats/json/parser/parser_state.hpp>
#include <userver/formats/json/parser/typed_parser.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

template <typename Item, typename ItemParser, typename Array = std::vector<Item>, auto... Validators>
class ArrayProxyParser final : public Subscriber<Array> {
public:
    using ResultType = Array;

    explicit ArrayProxyParser(ItemParser item_parser)
        : item_parser_(std::move(item_parser)), array_parser_(item_parser_) {
        array_parser_.Subscribe(*this);
    }

    ArrayProxyParser(ArrayProxyParser&& other) noexcept
        : item_parser_(std::move(other.item_parser_)), array_parser_(item_parser_), subscriber_(other.subscriber_) {
        array_parser_.Subscribe(*this);
        other.subscriber_ = nullptr;
    }

    ArrayProxyParser(ArrayProxyParser&) = delete;
    ArrayProxyParser& operator=(const ArrayProxyParser&) = delete;
    ArrayProxyParser& operator=(ArrayProxyParser&&) = delete;

    void Reset() { array_parser_.Reset(); }

    void Subscribe(Subscriber<Array>& subscriber) { subscriber_ = &subscriber; }

    void OnSend(Array&& result) override {
        if (subscriber_) {
            if constexpr (sizeof...(Validators) > 0) {
                (Validators(result), ...);
            }
            subscriber_->OnSend(std::move(result));
        }
    }

    auto& GetParser() { return array_parser_.GetParser(); }

    operator BaseParser&() { return array_parser_.GetParser(); }

private:
    ItemParser item_parser_;
    ArrayParser<Item, ItemParser, Array> array_parser_;
    Subscriber<Array>* subscriber_{nullptr};
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END