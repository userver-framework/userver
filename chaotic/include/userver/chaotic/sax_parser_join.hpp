#pragma once

#include <memory>

#include <userver/formats/json/parser/typed_parser.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::sax {

// TODO: move to formats::json::parse as ArrayParser?

template <typename ParserA, typename ParserB>
class JoinedParser final {
public:
    using ResultType = typename ParserA::ResultType;

    JoinedParser();
    ~JoinedParser();

    void Reset() { main_parser_.Reset(); }

    void Subscribe(formats::json::parser::Subscriber<typename ParserA::ResultType>& subscriber) {
        main_parser_.Subscribe(subscriber);
    }

    formats::json::parser::BaseParser& GetParser() { return main_parser_; }

private:
    // We use std::unique_ptr here to break the circular dependency for recursive types in the same way as it is
    // implicitly done in DOM.
    std::unique_ptr<ParserB> subparser_;
    ParserA main_parser_;
};

template <typename ParserA, typename ParserB>
JoinedParser<ParserA, ParserB>::JoinedParser()
    : subparser_(std::make_unique<ParserB>()),
      main_parser_(*subparser_)
{}

template <typename ParserA, typename ParserB>
JoinedParser<ParserA, ParserB>::~JoinedParser() = default;

}  // namespace chaotic::sax

USERVER_NAMESPACE_END
