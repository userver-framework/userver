#pragma once

/// @file userver/formats/json/parser/exception.hpp
/// @brief Exception types for JSON streaming parsers.
/// @ingroup userver_universal

#include <stdexcept>

#include <fmt/format.h>

#include <userver/formats/json/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

class BaseError : public formats::json::Exception {
public:
    using Exception::Exception;

    BaseError(const BaseError&) = default;
    BaseError(BaseError&&) = default;
    BaseError& operator=(const BaseError&) = default;
    BaseError& operator=(BaseError&&) = default;

    ~BaseError() override;
};

class ParseError : public BaseError {
public:
    ParseError(size_t pos, std::string_view path, std::string what)
        : BaseError(fmt::format("Parse error at pos {}, path '{}': {}", pos, path, what))
    {}

    ParseError(const ParseError&) = default;
    ParseError(ParseError&&) = default;
    ParseError& operator=(const ParseError&) = default;
    ParseError& operator=(ParseError&&) = default;

    ~ParseError() override;
};

class InternalParseError : public BaseError {
public:
    using BaseError::BaseError;

    InternalParseError(const InternalParseError&) = default;
    InternalParseError(InternalParseError&&) = default;
    InternalParseError& operator=(const InternalParseError&) = default;
    InternalParseError& operator=(InternalParseError&&) = default;

    ~InternalParseError() override;
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
