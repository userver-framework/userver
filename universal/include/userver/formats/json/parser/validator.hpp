#pragma once

#include <utility>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

template <typename T>
class BaseValidator {
 public:
  virtual void operator()(const T& t) const = 0;
};

template <typename T, typename F>
class Validator final : public BaseValidator<T> {
 public:
  explicit Validator(F f) : f_(std::move(f)) {}

  void operator()(const T& t) const override { f_(t); }

 private:
  F f_;
};

template <typename T>
class EmptyValidator final : public BaseValidator<T> {
 public:
  void operator()(const T&) const override {}
};

template <typename T, typename F>
auto MakeValidator(F f) {
  return Validator<T, F>(std::move(f));
}

template <typename T>
inline constexpr EmptyValidator<T> kEmptyValidator;

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
