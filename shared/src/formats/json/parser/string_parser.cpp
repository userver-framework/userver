#include <formats/json/parser/string_parser.hpp>

namespace formats::json::parser {

void StringParser::String(std::string_view sw) { SetResult(std::string{sw}); }

std::string StringParser::Expected() const { return "string"; }

}  // namespace formats::json::parser
