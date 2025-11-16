#pragma once

#include <userver/formats/json/parser/base_parser.hpp>
#include <userver/formats/json/parser/parser_state.hpp>
#include <userver/formats/json/parser/typed_parser.hpp>
#include <userver/formats/json/parser/validator.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

template <typename T, typename PrimitiveParser, auto... Validators>
class PrimitiveProxyParser final : public Subscriber<T> {
public:
    using ResultType = T;

    PrimitiveProxyParser() : primitive_parser_() { primitive_parser_.Subscribe(*this); }

    PrimitiveProxyParser(PrimitiveProxyParser&& other) noexcept
        : primitive_parser_(std::move(other.primitive_parser_)), subscriber_(other.subscriber_) {
        primitive_parser_.Subscribe(*this);
        other.subscriber_ = nullptr;
    }

    PrimitiveProxyParser(const PrimitiveProxyParser&) = delete;
    PrimitiveProxyParser& operator=(const PrimitiveProxyParser&) = delete;

    void Reset() { primitive_parser_.Reset(); }

    void Subscribe(Subscriber<T>& subscriber) { subscriber_ = &subscriber; }

    void OnSend(T&& value) override {
        if (subscriber_) {
            if constexpr (sizeof...(Validators) > 0){
                (Validators(value), ...);
            }
            subscriber_->OnSend(std::move(value));
        }
    }

    auto& GetParser() { return primitive_parser_.GetParser(); }

    operator BaseParser&() { return primitive_parser_.GetParser(); }

private:
    PrimitiveParser primitive_parser_;
    Subscriber<T>* subscriber_{nullptr};
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END