#pragma once

/// @file userver/utils/expected.hpp
/// @brief @copybrief utils::expected

#include <stdexcept>
#include <string>
#include <variant>

USERVER_NAMESPACE_BEGIN

namespace utils {

class bad_expected_access : public std::exception {
 public:
  using std::exception::exception;

  explicit bad_expected_access(const std::string& message)
      : message_{message} {}

  const char* what() const noexcept override { return message_.c_str(); }

 private:
  std::string message_;
};

template <class E>
class unexpected {
 public:
  unexpected(const E& error);
  unexpected(E&& error);

  template <class... Args>
  unexpected(Args&&... args);

  template <class U, class... Args>
  unexpected(std::initializer_list<U> il, Args&&... args);

  E& error() noexcept;
  const E& error() const noexcept;

 private:
  E value_;
};

template <class E>
unexpected(E) -> unexpected<E>;

/// @ingroup userver_containers
///
/// @brief For holding a value or an error
template <class S, class E>
class [[nodiscard]] expected {
 public:
  expected(const S& success);
  expected(S&& success);
  expected(const unexpected<E>& error);
  expected(unexpected<E>&& error);

  template <class G, typename = std::enable_if_t<std::is_convertible_v<G, E>>>
  expected(const unexpected<G>& error);

  template <class G, typename = std::enable_if_t<std::is_convertible_v<G, E>>>
  expected(unexpected<G>&& error);

  /// @brief Check whether *this contains an expected value
  bool has_value() const noexcept;

  /// @brief Return reference to the value or throws bad_expected_access
  /// if it's not available
  /// @throws utils::bad_expected_access if *this contain an unexpected value
  S& value();

  const S& value() const;

  /// @brief Return reference to the error value or throws bad_expected_access
  /// if it's not available
  /// @throws utils::bad_expected_access if success value is not available
  E& error();

  const E& error() const;

 private:
  std::variant<S, unexpected<E>> data_;
};

template <class E>
unexpected<E>::unexpected(const E& error) : value_{error} {}

template <class E>
unexpected<E>::unexpected(E&& error) : value_{std::forward<E>(error)} {}

template <class E>
template <class... Args>
unexpected<E>::unexpected(Args&&... args)
    : value_(std::forward<Args>(args)...) {}

template <class E>
template <class U, class... Args>
unexpected<E>::unexpected(std::initializer_list<U> il, Args&&... args)
    : value_(il, std::forward<Args>(args)...) {}

template <class E>
E& unexpected<E>::error() noexcept {
  return value_;
}

template <class E>
const E& unexpected<E>::error() const noexcept {
  return value_;
}

template <class S, class E>
expected<S, E>::expected(const S& success) : data_(success) {}

template <class S, class E>
expected<S, E>::expected(S&& success) : data_(std::forward<S>(success)) {}

template <class S, class E>
expected<S, E>::expected(const unexpected<E>& error) : data_(error.error()) {}

template <class S, class E>
expected<S, E>::expected(unexpected<E>&& error)
    : data_(std::forward<unexpected<E>>(error.error())) {}

template <class S, class E>
template <class G, typename>
expected<S, E>::expected(const unexpected<G>& error)
    : data_(utils::unexpected<E>(std::forward<G>(error.error()))) {}

template <class S, class E>
template <class G, typename>
expected<S, E>::expected(unexpected<G>&& error)
    : data_(utils::unexpected<E>(std::forward<G>(error.error()))) {}

template <class S, class E>
bool expected<S, E>::has_value() const noexcept {
  return std::holds_alternative<S>(data_);
}

template <class S, class E>
S& expected<S, E>::value() {
  S* result = std::get_if<S>(&data_);
  if (result == nullptr) {
    throw bad_expected_access(
        "Trying to get undefined value from utils::expected");
  }
  return *result;
}

template <class S, class E>
const S& expected<S, E>::value() const {
  const S* result = std::get_if<S>(&data_);
  if (result == nullptr) {
    throw bad_expected_access(
        "Trying to get undefined value from utils::expected");
  }
  return *result;
}

template <class S, class E>
E& expected<S, E>::error() {
  unexpected<E>* result = std::get_if<unexpected<E>>(&data_);
  if (result == nullptr) {
    throw bad_expected_access(
        "Trying to get undefined error value from utils::expected");
  }
  return result->error();
}

template <class S, class E>
const E& expected<S, E>::error() const {
  const unexpected<E>* result = std::get_if<unexpected<E>>(data_);
  if (result == nullptr) {
    throw bad_expected_access(
        "Trying to get undefined error value from utils::expected");
  }
  return result->error();
}

}  // namespace utils

USERVER_NAMESPACE_END
