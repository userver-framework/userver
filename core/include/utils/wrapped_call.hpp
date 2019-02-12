#pragma once

/// @file utils/wrapped_call.hpp
/// @brief @copybrief utils::WrappedCall

#include <exception>
#include <memory>
#include <tuple>

#include <utils/result_store.hpp>

namespace utils {
namespace impl {

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

  WrappedCall(WrappedCall&&) = delete;
  WrappedCall(const WrappedCall&) = delete;
  WrappedCall& operator=(WrappedCall&&) = delete;
  WrappedCall& operator=(const WrappedCall&) = delete;

 protected:
  using CallImpl = T (*)(WrappedCall* self);

  // No `virtual` to avoid vtable bloat and typeinfo generation.
  WrappedCall(CallImpl call) : call_impl_{call} {}
  ~WrappedCall() = default;  // Must be overriden in derived class.

 private:
  void DoPerform();
  T Call() { return call_impl_(this); }

  const CallImpl call_impl_;
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

// Stores passed arguments and function. Invokes function later with argument
// types exactly matching the initial types of arguments passed to WrapCall.
template <typename Function, typename... Args>
class WrappedCallImpl final
    : public WrappedCall<decltype(
          std::declval<Function>()(std::declval<Args>()...))> {
  using ResultType =
      decltype(std::declval<Function>()(std::declval<Args>()...));
  using BaseType = WrappedCall<ResultType>;

 public:
  explicit WrappedCallImpl(Function&& f, Args&&... args)
      : BaseType(&WrappedCallImpl::Call),
        f_(std::forward<Function>(f)),
        args_(std::make_tuple(std::forward<Args>(args)...)) {}

 private:
  static ResultType Call(BaseType* self) {
    return static_cast<WrappedCallImpl*>(self)->DoCall(
        std::index_sequence_for<Args...>());
  }

  template <size_t... Indices>
  ResultType DoCall(std::index_sequence<Indices...>) {
    if constexpr (std::is_pointer<StoredFunction>::value) {
      assert(f_);
      auto f = std::exchange(f_, nullptr);
      return f(std::forward<Args>(std::get<Indices>(args_))...);
    } else {
      // We have to cleanup `f_` after usage. Moving it to temporary.
      auto f = std::move(f_);
      return std::forward<Function>(f)(
          std::forward<Args>(std::get<Indices>(args_))...);
    }
  }

  // Storing function references as function pointers, storing other function
  // types (including function objects) by value.
  using UnrefFunction = std::remove_reference_t<Function>;
  using StoredFunction =
      std::conditional_t<std::is_function<UnrefFunction>::value, UnrefFunction*,
                         UnrefFunction>;
  StoredFunction f_;
  decltype(std::make_tuple(std::declval<Args>()...)) args_;
};

}  // namespace impl

template <typename Function, typename... Args>
auto WrapCall(Function&& f, Args&&... args) {
  static_assert(
      (!std::is_array<std::remove_reference_t<Args>>::value && ...),
      "Passing C arrays to Async is forbidden. Use std::array instead");

  return std::make_shared<impl::WrappedCallImpl<Function, Args...>>(
      std::forward<Function>(f), std::forward<Args>(args)...);
}

}  // namespace utils
