#pragma once

/// @file userver/utils/wrapped_call.hpp
/// @brief @copybrief utils::WrappedCall

#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/result_store.hpp>

namespace utils::impl {

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
  using CallImpl = T (*)(WrappedCall& self);

  // No `virtual` to avoid vtable bloat and typeinfo generation.
  explicit WrappedCall(CallImpl call) : call_impl_{call} {}
  ~WrappedCall() = default;  // Must be overridden in derived class.

 private:
  void DoPerform();
  T Call() { return call_impl_(*this); }

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

template <typename T>
class OptionalSetNoneGuard final {
 public:
  OptionalSetNoneGuard(std::optional<T>& o) noexcept : o_(o) {}

  ~OptionalSetNoneGuard() { o_ = std::nullopt; }

 private:
  std::optional<T>& o_;
};

template <typename T>
struct UnrefImpl final {
  using type = T;
};

template <typename T>
struct UnrefImpl<std::reference_wrapper<T>> final {
  using type = T&;
};

template <typename T>
using DecayUnref = typename UnrefImpl<std::decay_t<T>>::type;

// Stores passed arguments and function. Invokes function later with argument
// types exactly matching the initial types of arguments passed to WrapCall.
template <typename Function, typename... Args>
class WrappedCallImpl final
    : public WrappedCall<std::invoke_result_t<Function&&, Args&&...>> {
  using ResultType = std::invoke_result_t<Function&&, Args&&...>;
  using BaseType = WrappedCall<ResultType>;

 public:
  template <typename RawFunction, typename RawArgsTuple>
  explicit WrappedCallImpl(RawFunction&& func, RawArgsTuple&& args)
      : BaseType(&WrappedCallImpl::Call),
        data_(std::in_place, std::forward<RawFunction>(func),
              std::forward<RawArgsTuple>(args)) {}

  ~WrappedCallImpl() = default;

 private:
  static ResultType Call(BaseType& self) {
    auto& data = static_cast<WrappedCallImpl&>(self).data_;

    UASSERT(data);
    // NOLINTNEXTLINE(bugprone-suspicious-semicolon)
    if constexpr (std::is_pointer_v<Function>) {
      UASSERT(data->func != nullptr);
    }

    OptionalSetNoneGuard guard(data);

    // This is the point at which stacktrace is cut,
    // see 'logging/stacktrace_cache.cpp'.
    return std::apply(std::forward<Function>(data->func),
                      std::move(data->args));
  }

  struct Data final {
    // TODO remove after paren-init for aggregates in C++20
    template <typename RawFunction, typename RawArgsTuple>
    explicit Data(RawFunction&& func, RawArgsTuple&& args)
        : func(std::forward<RawFunction>(func)),
          args(std::forward<RawArgsTuple>(args)) {}

    Function func;
    std::tuple<Args...> args;
  };

  std::optional<Data> data_;
};

/// Returns an object that stores passed arguments and function. Wrapped
/// function may be invoked only once via call to member function Perform().
template <typename Function, typename... Args>
auto WrapCall(Function&& f, Args&&... args) {
  static_assert(
      (!std::is_array_v<std::remove_reference_t<Args>> && ...),
      "Passing C arrays to Async is forbidden. Use std::array instead");

  return std::make_shared<impl::WrappedCallImpl<impl::DecayUnref<Function>,
                                                impl::DecayUnref<Args>...>>(
      std::forward<Function>(f),
      std::forward_as_tuple(std::forward<Args>(args)...));
}

}  // namespace utils::impl
