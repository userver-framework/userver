#pragma once

/// @file utils/result_store.hpp
/// @brief @copybrief utils::ResultStore

#include <exception>
#include <optional>
#include <stdexcept>

#include <boost/optional.hpp>

namespace utils {

/// Simple value/exception store
template <typename T>
class ResultStore final {
 public:
  /// @brief Retrieves the stored value or rethrows the stored exception
  /// @throws std::logic_error if no value/exception stored
  /// @note Can be called at most once.
  T Retrieve();

  /// Stores a value
  void SetValue(const T&);

  /// Stores a value
  void SetValue(T&&);

  /// Stores an exception
  void SetException(std::exception_ptr&&) noexcept;

 private:
  // variant here would require a specialization for exception_ptr
  std::optional<T> value_;
  std::exception_ptr exception_;
};

/// Simple void value/exception store
template <>
class ResultStore<void> final {
 public:
  /// @brief Checks value availability or rethrows the stored exception
  /// @throws std::logic_error if no value/exception stored
  void Retrieve();

  /// Marks the value as available
  void SetValue() noexcept;

  /// Stores an exception
  void SetException(std::exception_ptr&&) noexcept;

 private:
  bool has_value_{false};
  std::exception_ptr exception_;
};

template <typename T>
T ResultStore<T>::Retrieve() {
  if (value_) return std::move(*value_);
  if (exception_) std::rethrow_exception(exception_);
  throw std::logic_error("result store is not ready");
}

template <typename T>
void ResultStore<T>::SetValue(const T& value) {
  value_ = value;
}

template <typename T>
void ResultStore<T>::SetValue(T&& value) {
  value_.emplace(std::move(value));
}

template <typename T>
void ResultStore<T>::SetException(std::exception_ptr&& exception) noexcept {
  exception_ = std::move(exception);
}

inline void ResultStore<void>::Retrieve() {
  if (has_value_) return;
  if (exception_) std::rethrow_exception(exception_);
  throw std::logic_error("result store is not ready");
}

inline void ResultStore<void>::SetValue() noexcept { has_value_ = true; }

inline void ResultStore<void>::SetException(
    std::exception_ptr&& exception) noexcept {
  exception_ = std::move(exception);
}

}  // namespace utils
