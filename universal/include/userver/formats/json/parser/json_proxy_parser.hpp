#pragma once

#include <userver/formats/json/parser/parser_json.hpp>
#include <userver/formats/json/parser/typed_parser.hpp>
#include <userver/formats/json/parser/validator.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

template <typename T, auto... Validators>
class JsonValueProxyParser final : public Subscriber<Value> {
public:
    using ResultType = T;

    JsonValueProxyParser() : value_parser_() { value_parser_.Subscribe(*this); }

    JsonValueProxyParser(JsonValueProxyParser&& other) noexcept
        : value_parser_(std::move(other.value_parser_)), subscriber_(other.subscriber_){
        value_parser_.Subscribe(*this);
        other.subscriber_ = nullptr;
    }

    JsonValueProxyParser(const JsonValueProxyParser&) = delete;
    JsonValueProxyParser& operator=(const JsonValueProxyParser&) = delete;

    void Reset() { value_parser_.Reset(); }

    void Subscribe(Subscriber<T>& subscriber) { subscriber_ = &subscriber; }

    void OnSend(Value&& value) override {
        if (subscriber_) {
            try {
                T result = value.As<T>();
                if constexpr (sizeof...(Validators) > 0) {
                    (Validators(result), ...);
                }
                subscriber_->OnSend(std::move(result));
            } catch (const std::exception& e) {
                throw InternalParseError(std::string("Failed to convert json value to type: ") + e.what());
            }
        }
    }

    operator BaseParser&() { return value_parser_.GetParser(); }

    BaseParser& GetParser() { return value_parser_.GetParser(); }

private:
    JsonValueParser value_parser_;
    Subscriber<T>* subscriber_{nullptr};
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END