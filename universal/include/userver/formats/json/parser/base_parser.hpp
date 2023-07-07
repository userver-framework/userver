#pragma once

#include <userver/formats/json/parser/exception.hpp>
#include <userver/formats/json/parser/parser_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

/// @brief Base class for SAX parser.
///
class BaseParser {
 public:
  virtual ~BaseParser() = default;

  virtual void Null() { Throw("null"); }
  virtual void Bool(bool) { Throw("bool"); }
  virtual void Int64(int64_t) { Throw("integer"); }
  virtual void Uint64(uint64_t) { Throw("integer"); }
  virtual void Double(double) { Throw("double"); }
  virtual void String(std::string_view) { Throw("string"); }
  virtual void StartObject() { Throw("object"); }
  virtual void Key(std::string_view key) {
    Throw("field '" + std::string(key) + "'");
  }
  virtual void EndObject() { Throw("'}'"); }
  virtual void StartArray() { Throw("array"); }
  virtual void EndArray() { Throw("']'"); }

  // Low-level variants of EndObject/EndArray
  virtual void EndObject(size_t /* members */) { EndObject(); }
  virtual void EndArray(size_t /* members */) { EndArray(); }

  void SetState(ParserState& state) { parser_state_ = &state; }

  virtual std::string GetPathItem() const = 0;

 protected:
  [[noreturn]] void Throw(const std::string& found) {
    throw InternalParseError(Expected() + " was expected, but " + found +
                             " found");
  }

  virtual std::string Expected() const = 0;

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  ParserState* parser_state_{nullptr};
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
