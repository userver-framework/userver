#pragma once

#include <formats/json/parser/base_parser.hpp>
#include <utils/assert.hpp>

namespace formats::json::parser {

class Subscriber {
 public:
  virtual ~Subscriber() = default;

  virtual void OnSend() = 0;
};

template <typename T>
class TypedParser : public BaseParser {
 public:
  void Reset(T& result) { result_ = &result; }

  void Subscribe(Subscriber& subscriber) { subscriber_ = &subscriber; }

  using ResultType = T;

 protected:
  void SetResult(T value) {
    UASSERT(result_);
    *result_ = std::move(value);
    if (subscriber_) subscriber_->OnSend();
  }

  void Validate() const { UASSERT(result_); }

  void PopAndValidate() { parser_state_->PopMe(*this); }

  T* result_{nullptr};
  Subscriber* subscriber_{nullptr};
};

}  // namespace formats::json::parser
