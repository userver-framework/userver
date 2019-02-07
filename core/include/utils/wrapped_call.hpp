#pragma once

/// @file utils/wrapped_call.hpp
/// @brief @copybrief utils::WrappedCall

#include <exception>
#include <memory>
#include <tuple>

#include <utils/result_store.hpp>

namespace utils {

/// Wraps a function invocation into a WrappedCall holder
template <typename Function, typename... Args>
auto WrapCall(Function&& f, Args&&... args);

/// std::packaged_task replacement with noncopyable types support
template <typename T>
class WrappedCall {
 public:
  /// Invokes the wrapped function call
  void Perform();

  /// Returns (or rethrows) the result of wrapped call invocation
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
  return result_.Retrieve();
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

namespace impl {

template <typename Function, typename... Args>
class WrappedCallImpl final
    : public WrappedCall<decltype(
          std::declval<Function>()(std::declval<Args>()...))> {
 public:
  template <class... ArgsIn>
  explicit WrappedCallImpl(Function&& f, ArgsIn&&... args)
      : f_(std::forward<Function>(f)),
        args_(std::make_tuple(std::forward<ArgsIn>(args)...)) {}

 private:
  decltype(std::declval<Function>()(std::declval<Args>()...)) Call() override {
    return DoCall(std::index_sequence_for<Args...>());
  }

  template <size_t... Indices>
  decltype(auto) DoCall(std::index_sequence<Indices...>) {
    auto f = std::move(f_);
    return f(std::get<Indices>(std::move(args_))...);
  }

  Function f_;
  decltype(std::make_tuple(std::declval<Args>()...)) args_;
};

template <class T>
using remove_cv_ref_t = std::remove_cv_t<std::remove_reference_t<T>>;

}  // namespace impl

template <typename Function, typename... Args>
auto WrapCall(Function&& f, Args&&... args) {
  static_assert((!std::is_array<impl::remove_cv_ref_t<Args>>::value && ...),
                "Passing arrays to Async is forbidden");

  return std::make_shared<
      impl::WrappedCallImpl<Function, impl::remove_cv_ref_t<Args>...>>(
      std::forward<Function>(f), std::forward<Args>(args)...);
}

}  // namespace utils
