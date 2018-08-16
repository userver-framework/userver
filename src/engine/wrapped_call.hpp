#pragma once

#include <exception>
#include <memory>
#include <tuple>

#include "result_store.hpp"

namespace engine {
namespace impl {

template <typename Function, typename... Args>
auto WrapCall(Function&& f, Args&&... args);

template <typename T>
class WrappedCall {
 public:
  void Perform();
  T Retrieve();

 protected:
  virtual ~WrappedCall() = default;

 private:
  void DoPerform();

  virtual T Call() = 0;

  ResultStore<T> result_;
};

template <typename T>
void WrappedCall<T>::Perform() {
  try {
    DoPerform();
  } catch (const std::exception&) {
    result_.SetException(std::current_exception());
  }
}

template <typename T>
T WrappedCall<T>::Retrieve() {
  return result_.Get();
}

template <typename T>
void WrappedCall<T>::DoPerform() {
  result_.SetValue(Call());
}

template <>
inline void WrappedCall<void>::DoPerform() {
  Call();
  result_.SetValue();
}

template <typename Function, typename... Args>
class WrappedCallImpl final
    : public WrappedCall<decltype(
          std::declval<Function>()(std::declval<Args>()...))> {
 public:
  explicit WrappedCallImpl(Function&& f, Args&&... args)
      : f_(std::move(f)), args_(std::make_tuple(std::forward<Args>(args)...)) {}

 private:
  decltype(std::declval<Function>()(std::declval<Args>()...)) Call() override {
    return DoCall(std::index_sequence_for<Args...>());
  }

  template <size_t... Indices>
  decltype(auto) DoCall(std::index_sequence<Indices...>) {
    return f_(std::get<Indices>(std::move(args_))...);
  }

  Function f_;
  decltype(std::make_tuple(std::declval<Args>()...)) args_;
};

template <typename Function, typename... Args>
auto WrapCall(Function&& f, Args&&... args) {
  return std::make_shared<WrappedCallImpl<Function, Args...>>(
      std::forward<Function>(f), std::forward<Args>(args)...);
}

}  // namespace impl
}  // namespace engine
