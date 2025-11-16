#pragma once

#include <userver/formats/json/parser/object_parser.hpp>
#include <userver/formats/json/parser/typed_parser.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

template <typename T, auto... Validators>
class ObjectProxyParser final : public Subscriber<T> {
public:
    using ResultType = T;
    ObjectProxyParser() { parser_.Subscribe(*this); }
    ObjectProxyParser(ObjectProxyParser&& other) noexcept
        : parser_(),
          subscriber_(other.subscriber_) 
    {
        parser_.Subscribe(*this);
        other.subscriber_ = nullptr;
    }
    ObjectProxyParser(const ObjectProxyParser&) = delete;
    ObjectProxyParser& operator=(const ObjectProxyParser&) = delete;
    ObjectProxyParser& operator=(ObjectProxyParser&&) = delete;

    void Reset() { parser_.Reset(); }
    void Subscribe(Subscriber<T>& subscriber) { subscriber_ = &subscriber; }

    void OnSend(T&& result) override {
        if (subscriber_) {
            if constexpr (sizeof...(Validators) > 0) {
                (Validators(result), ...);
            }
            subscriber_->OnSend(std::move(result));
        }
    }

    BaseParser& GetParser() { return parser_; }
    
    operator BaseParser&() { return parser_; }

private:
    ObjectParser<T> parser_;
    Subscriber<T>* subscriber_{nullptr};
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END