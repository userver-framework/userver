#pragma once

#include <userver/formats/json/parser/map_parser.hpp>
#include <userver/formats/json/parser/typed_parser.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

template <typename Map, typename ValueParser, auto... Validators>
class MapProxyParser final : public Subscriber<Map> {
public:
    using ResultType = Map;

    explicit MapProxyParser(ValueParser value_parser)
        : value_parser_(std::move(value_parser)), map_parser_(value_parser_) {
        map_parser_.Subscribe(*this);
    }

    MapProxyParser(MapProxyParser&& other) noexcept
        : value_parser_(std::move(other.value_parser_)), map_parser_(value_parser_), subscriber_(other.subscriber_) {
        map_parser_.Subscribe(*this);
        other.subscriber_ = nullptr;
    }

    MapProxyParser(MapProxyParser&) = delete;
    MapProxyParser& operator=(const MapProxyParser&) = delete;
    MapProxyParser& operator=(MapProxyParser&&) = delete;

    void Reset() { map_parser_.Reset(); }

    void Subscribe(Subscriber<Map>& subscriber) { subscriber_ = &subscriber; }

    void OnSend(Map&& result) override {
        if (subscriber_) {
            if constexpr (sizeof...(Validators) > 0) {
                (Validators(result), ...);
            }
            subscriber_->OnSend(std::move(result));
        }
    }

    auto& GetParser() { return map_parser_.GetParser(); }

    operator BaseParser&() { return map_parser_.GetParser(); }

private:
    ValueParser value_parser_;
    MapParser<Map, ValueParser> map_parser_;
    Subscriber<Map>* subscriber_{nullptr};
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END