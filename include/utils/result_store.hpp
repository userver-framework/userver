#pragma once

#include <exception>
#include <stdexcept>

#include <boost/optional.hpp>

namespace utils {

template <typename T>
class ResultStore {
 public:
  T Retrieve();

  void SetValue(const T&);
  void SetValue(T&&);
  void SetException(std::exception_ptr&&);

 private:
  // variant here would require a specialization for exception_ptr
  boost::optional<T> value_;
  std::exception_ptr exception_;
};

template <>
class ResultStore<void> {
 public:
  void Retrieve();

  void SetValue();
  void SetException(std::exception_ptr&&);

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
void ResultStore<T>::SetException(std::exception_ptr&& exception) {
  exception_ = std::move(exception);
}

inline void ResultStore<void>::Retrieve() {
  if (has_value_) return;
  if (exception_) std::rethrow_exception(exception_);
  throw std::logic_error("result store is not ready");
}

inline void ResultStore<void>::SetValue() { has_value_ = true; }

inline void ResultStore<void>::SetException(std::exception_ptr&& exception) {
  exception_ = std::move(exception);
}

}  // namespace utils
