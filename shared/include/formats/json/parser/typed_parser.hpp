#pragma once

#include <formats/json/parser/base_parser.hpp>
#include <formats/json/parser/validator.hpp>
#include <utils/assert.hpp>

namespace formats::json::parser {

template <typename T>
class TypedParser : public BaseParser {
 public:
  void Reset(T& result) { result_ = &result; }

  TypedParser() : validator_(kEmptyValidator<T>) {}

  explicit TypedParser(const BaseValidator<T>& validator)
      : validator_(validator) {}

  using ResultType = T;

 protected:
  void SetResult(T value) {
    UASSERT(result_);
    *result_ = std::move(value);
  }

  void Validate() const {
    UASSERT(result_);
    validator_(*result_);
  }

  void PopAndValidate() {
    parser_state_->PopMe(*this);
    Validate();
  }

  T* result_{nullptr};
  const BaseValidator<T>& validator_;
};

}  // namespace formats::json::parser
