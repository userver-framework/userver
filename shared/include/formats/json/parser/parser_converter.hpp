#pragma once

#include <formats/json/parser/typed_parser.hpp>
#include <formats/parse/to.hpp>

namespace formats::json::parser {

template <typename FinalType, typename Parser>
class ParserConverter final {
 public:
  static auto MakeParserValidator(
      const BaseValidator<typename Parser::ResultType>& user_validator,
      FinalType** final_result) {
    return MakeValidator<typename Parser::ResultType>(
        [final_result,
         &user_validator](const typename Parser::ResultType& value) {
          user_validator(value);
          **final_result = Parse(value, ::formats::parse::To<FinalType>());
        });
  }

  ParserConverter(
      const BaseValidator<typename Parser::ResultType>& user_validator =
          kEmptyValidator<typename Parser::ResultType>)
      : validator_(MakeParserValidator(user_validator, &result_)),
        parser_(validator_) {}

  void Reset(FinalType& result) {
    result_ = &result;
    raw_result_ = {};
    parser_.Reset(raw_result_);
  }

  operator Parser&() { return parser_; }

 private:
  FinalType* result_{nullptr};
  typename Parser::ResultType raw_result_;
  decltype(MakeParserValidator(kEmptyValidator<typename Parser::ResultType>,
                               std::declval<FinalType**>())) validator_;
  Parser parser_;
};

}  // namespace formats::json::parser
