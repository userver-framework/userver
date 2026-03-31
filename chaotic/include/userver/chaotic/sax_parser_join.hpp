#pragma once

#include <userver/formats/json/parser/typed_parser.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::sax {

// TODO: move to formats::json::parse as ArrayParser?

template <typename ParserA, typename ParserB>
class JoinedParser final {
public:
    using ResultType = typename ParserA::ResultType;

    JoinedParser()
        : main_parser_(subparser_)
    {}

    void Reset() { main_parser_.Reset(); }

    void Subscribe(formats::json::parser::Subscriber<typename ParserA::ResultType>& subscriber) {
        main_parser_.Subscribe(subscriber);
    }

    auto& GetParser() { return main_parser_; }

    operator formats::json::parser::BaseParser&()
    {
        return main_parser_;
    }

private:
    ParserB subparser_;
    ParserA main_parser_;
};

}  // namespace chaotic::sax

USERVER_NAMESPACE_END
