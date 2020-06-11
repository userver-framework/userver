#include <formats/json/parser/int_parser.hpp>

#include <boost/numeric/conversion/cast.hpp>

namespace formats::json::parser {

void IntParser::Int64(int64_t i) {
  SetResult(i);
  PopAndValidate();
}

void IntParser::Uint64(uint64_t i) { Int64(boost::numeric_cast<int64_t>(i)); }

std::string IntParser::Expected() const { return "integer"; }

}  // namespace formats::json::parser
