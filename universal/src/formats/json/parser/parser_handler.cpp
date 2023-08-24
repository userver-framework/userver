#include <userver/formats/json/parser/parser_handler.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

ParserHandler::ParserHandler(BaseParser& parser) : parser_(parser) {}

ParserHandler::ParserHandler(ParserState& parser_state)
    : ParserHandler(parser_state.GetTopParser()) {}

bool ParserHandler::Null() {
  parser_.Null();
  return true;
}
bool ParserHandler::Bool(bool b) {
  parser_.Bool(b);
  return true;
}
bool ParserHandler::Int(int64_t i) { return Int64(i); }
bool ParserHandler::Uint(uint64_t u) { return Uint64(u); }
bool ParserHandler::Int64(int64_t i) {
  parser_.Int64(i);
  return true;
}
bool ParserHandler::Uint64(uint64_t u) {
  parser_.Uint64(u);
  return true;
}
bool ParserHandler::Double(double d) {
  parser_.Double(d);
  return true;
}
bool ParserHandler::StartObject() {
  parser_.StartObject();
  return true;
}
bool ParserHandler::EndObject(size_t members) {
  parser_.EndObject(members);
  return true;
}
bool ParserHandler::StartArray() {
  parser_.StartArray();
  return true;
}
bool ParserHandler::EndArray(size_t members) {
  parser_.EndArray(members);
  return true;
}

bool ParserHandler::Key(const char* c, size_t size, bool) {
  parser_.Key(std::string_view{c, size});
  return true;
}

bool ParserHandler::String(const char* c, size_t size, bool) {
  parser_.String(std::string_view{c, size});
  return true;
}

// no, this is a trait member function
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool ParserHandler::RawNumber(const char*, size_t, bool) { return false; }

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
