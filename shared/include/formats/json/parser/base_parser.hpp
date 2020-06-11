#pragma once

#include <formats/json/parser/exception.hpp>
#include <formats/json/parser/parser_state.hpp>

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

  void SetState(ParserState& state) { parser_state_ = &state; }

 protected:
  [[noreturn]] void Throw(const std::string& found) {
    throw InternalParseError(Expected() + " was expected, but " + found +
                             " found");
  }

  virtual std::string Expected() const = 0;

  ParserState* parser_state_{nullptr};
};

template <typename T, typename Parser>
T ParseToType(std::string_view input) {
  T result{};
  Parser parser;
  parser.Reset(result);

  ParserState state;
  state.PushParserNoKey(parser);
  state.ProcessInput(input);

  return result;
}

}  // namespace formats::json::parser
