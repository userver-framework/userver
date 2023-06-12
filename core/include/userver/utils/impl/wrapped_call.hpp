#pragma once

/// @file userver/utils/impl/wrapped_call.hpp
/// @brief @copybrief utils::impl::WrappedCall

#include <cstddef>
#include <functional>
#include <new>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

#include <userver/utils/assert.hpp>
#include <userver/utils/impl/wrapped_call_base.hpp>
#include <userver/utils/lazy_prvalue.hpp>
#include <userver/utils/result_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

/// std::packaged_task replacement with noncopyable types support
template <typename T>
class WrappedCall : public WrappedCallBase {
 public:
  /// Returns (or rethrows) the result of wrapped call invocation
  T Retrieve() { return result_.Retrieve(); }

  /// Returns (or rethrows) the result of wrapped call invocation
  decltype(auto) Get() const& { return result_.Get(); }

  void RethrowErrorResult() const final { (void)result_.Get(); }

 protected:
  WrappedCall() noexcept = default;

  ResultStore<T>& GetResultStore() noexcept { return result_; }

 private:
  ResultStore<T> result_;
};

template <typename T>
WrappedCall<T>& CastWrappedCall(WrappedCallBase& wrapped_call) noexcept {
  UASSERT(dynamic_cast<WrappedCall<T>*>(&wrapped_call) != nullptr);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  return static_cast<WrappedCall<T>&>(wrapped_call);
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

template <typename Func>
struct UnrefImpl<utils::LazyPrvalue<Func>> final {
  using type = std::invoke_result_t<Func&&>;
};

template <typename T>
using DecayUnref = typename UnrefImpl<std::decay_t<T>>::type;

// Stores passed arguments and function. Invokes function later with argument
// types exactly matching the initial types of arguments passed to WrapCall.
template <typename Function, typename... Args>
class WrappedCallImpl final
    : public WrappedCall<std::invoke_result_t<Function&&, Args&&...>> {
 public:
  using ResultType = std::invoke_result_t<Function&&, Args&&...>;

  template <typename RawFunction, typename RawArgsTuple>
  explicit WrappedCallImpl(RawFunction&& func, RawArgsTuple&& args)
      : data_(std::in_place, std::forward<RawFunction>(func),
              std::forward<RawArgsTuple>(args)) {}

  void Perform() override {
    UASSERT(data_);
    if constexpr (std::is_pointer_v<Function>) {
      UASSERT(data_->func != nullptr);
    }

    OptionalSetNoneGuard guard(data_);
    auto& result = this->GetResultStore();

    // This is the point at which stacktrace is cut,
    // see 'logging/stacktrace_cache.cpp'.
    try {
      if constexpr (std::is_void_v<ResultType>) {
        std::apply(std::forward<Function>(data_->func), std::move(data_->args));
        result.SetValue();
      } else {
        result.SetValue(std::apply(std::forward<Function>(data_->func),
                                   std::move(data_->args)));
      }
    } catch (const std::exception&) {
      result.SetException(std::current_exception());
    }
  }

 private:
  struct Data final {
    // TODO remove after paren-init for aggregates in C++20
    template <typename RawFunction, typename RawArgsTuple>
    explicit Data(RawFunction&& func, RawArgsTuple&& args)
        // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
        : func(std::forward<RawFunction>(func)),
          args(std::forward<RawArgsTuple>(args)) {}

    Function func;
    std::tuple<Args...> args;
  };

  std::optional<Data> data_;
};

template <typename Function, typename... Args>
using WrappedCallImplType =
    WrappedCallImpl<DecayUnref<Function>, DecayUnref<Args>...>;

/// Construct a WrappedCallImplType at `storage`. See WrappedCall and
/// WrappedCallBase for the API of the result. Note: using `reinterpret_cast` to
/// cast `storage` to WrappedCallImpl is UB, use the function return value.
template <typename Function, typename... Args>
[[nodiscard]] auto& PlacementNewWrapCall(std::byte* storage, Function&& f,
                                         Args&&... args) {
  static_assert(
      (!std::is_array_v<std::remove_reference_t<Args>> && ...),
      "Passing C arrays to Async is forbidden. Use std::array instead");

  return *new (storage) WrappedCallImplType<Function, Args...>(
      std::forward<Function>(f),
      std::forward_as_tuple(std::forward<Args>(args)...));
}

}  // namespace utils::impl

USERVER_NAMESPACE_END
