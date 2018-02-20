#pragma once

#include <tuple>

namespace engine {
namespace impl {

template <typename T>
class CallWrapperBase {
 public:
  virtual ~CallWrapperBase() {}
  virtual T Call() = 0;
};

template <typename Function, typename... Args>
class CallWrapper : public CallWrapperBase<decltype(
                        std::declval<Function>()(std::declval<Args>()...))> {
 public:
  explicit CallWrapper(Function&& f, Args&&... args)
      : f_(std::move(f)), args_(std::make_tuple(std::forward<Args>(args)...)) {}

  decltype(std::declval<Function>()(std::declval<Args>()...)) Call() override {
    return DoCall(std::index_sequence_for<Args...>());
  }

 private:
  template <size_t... Indices>
  auto DoCall(std::index_sequence<Indices...>) {
    return f_(std::get<Indices>(std::move(args_))...);
  }

  Function f_;
  decltype(std::make_tuple(std::declval<Args>()...)) args_;
};

}  // namespace impl
}  // namespace engine
