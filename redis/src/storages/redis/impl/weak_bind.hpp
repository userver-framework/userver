#pragma once

#include <functional>
#include <stdexcept>

namespace utils {

/* Throw AsyncBreakError by CallPeriodicallyXXX's callback to stop periodic
 * calls. */
class AsyncBreakError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

template <class T, class F, bool throwable>
class WeakBind_ {
 public:
  WeakBind_(std::weak_ptr<T> ptr, F func) : ptr_(ptr), func_(std::move(func)) {}

  template <class... Args>
  void operator()(Args&&... args) const {
    auto obj = ptr_.lock();
    if (!obj) {
      if (throwable)
        throw AsyncBreakError("object is dead");
      else
        return;
    }

    func_(std::forward<Args>(args)...);
  }

 private:
  std::weak_ptr<T> ptr_;
  F func_;
};

template <class T, bool Throwable, class... Args, class... Args2>
auto WeakBindGeneric(void (T::*f)(Args2...), std::shared_ptr<T> obj,
                     Args... args)
    -> WeakBind_<T, decltype(std::bind(f, obj.get(), args...)), Throwable> {
  std::weak_ptr<T> weak = obj;
  auto func = std::bind(f, obj.get(), args...);
  return WeakBind_<T, decltype(func), Throwable>(weak, std::move(func));
}

template <class T, bool Throwable, class... Args, class... Args2>
auto WeakBindGeneric(void (T::*f)(Args2...) const, std::shared_ptr<const T> obj,
                     Args... args)
    -> WeakBind_<const T, decltype(std::bind(f, obj.get(), args...)),
                 Throwable> {
  std::weak_ptr<const T> weak = obj;
  auto func = std::bind(f, obj.get(), args...);
  return WeakBind_<const T, decltype(func), Throwable>(weak, std::move(func));
}

template <class T, class... Args, class... Args2>
auto WeakBind(void (T::*f)(Args2...), std::shared_ptr<T> obj, Args... args)
    -> WeakBind_<T, decltype(std::bind(f, obj.get(), args...)), false> {
  return WeakBindGeneric<T, false, Args...>(f, obj, args...);
}

template <class T, class... Args, class... Args2>
auto WeakBind(void (T::*f)(Args2...) const, std::shared_ptr<const T> obj,
              Args... args)
    -> WeakBind_<const T, decltype(std::bind(f, obj.get(), args...)), false> {
  return WeakBindGeneric<T, false, Args...>(f, obj, args...);
}

template <class T, class... Args, class... Args2>
auto WeakBindThrowable(void (T::*f)(Args2...), std::shared_ptr<T> obj,
                       Args... args)
    -> WeakBind_<T, decltype(std::bind(f, obj.get(), args...)), true> {
  return WeakBindGeneric<T, true, Args...>(f, obj, args...);
}

}  // namespace utils
