#pragma once

#include <stdexcept>

#include <fmt/format.h>

#include <userver/formats/json/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

class BaseError : public formats::json::Exception {
  using Exception::Exception;
};

class ParseError : public BaseError {
 public:
  ParseError(size_t pos, std::string_view path, std::string what)
      : BaseError(fmt::format("Parse error at pos {}, path '{}': {}", pos, path,
                              what)) {}
};

class InternalParseError : public BaseError {
  using BaseError::BaseError;
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
