#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

#include <userver/storages/mysql/impl/binder_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

template <typename BindsStorage>
class BinderBase {
 public:
  BinderBase(BindsStorage& binds, std::size_t pos) : binds_{binds}, pos_{pos} {}

  void operator()() { Bind(); }

 protected:
  virtual void Bind() = 0;

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  BindsStorage& binds_;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::size_t pos_;
};

template <typename Field>
class InputBinderBase : public BinderBase<InputBindingsFwd> {
 public:
  InputBinderBase(InputBindingsFwd& binds, std::size_t pos, const Field& field)
      : BinderBase(binds, pos), field_{field} {}

 protected:
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  const Field& field_;
};

template <typename Field>
class OutputBinderBase : public BinderBase<OutputBindingsFwd> {
 public:
  OutputBinderBase(OutputBindingsFwd& binds, std::size_t pos, Field& field)
      : BinderBase{binds, pos}, field_{field} {}

 protected:
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  Field& field_;
};

template <typename T>
class InputBinder final : public InputBinderBase<T> {
 public:
  using InputBinderBase<T>::InputBinderBase;

  void Bind() final {
    using SteadyClock = std::chrono::steady_clock;

    if constexpr (std::is_same_v<SteadyClock::time_point, T> ||
                  std::is_same_v<std::optional<SteadyClock::time_point>, T>) {
      static_assert(!sizeof(T),
                    "Don't store steady_clock times in the database, use "
                    "system_clock instead");
    } else {
      static_assert(!sizeof(T),
                    "Input binding for the type is not implemented");
    }
  }
};

template <typename T>
class OutputBinder final : public OutputBinderBase<T> {
 public:
  using OutputBinderBase<T>::OutputBinderBase;

  void Bind() final {
    using SteadyClock = std::chrono::steady_clock;
    if constexpr (std::is_same_v<SteadyClock::time_point, T> ||
                  std::is_same_v<std::optional<SteadyClock::time_point>, T>) {
      static_assert(!sizeof(T),
                    "Don't store steady_clock times in the database, use "
                    "system_clock instead");
    } else {
      static_assert(!sizeof(T),
                    "Output binding for the type is not implemented");
    }
  }
};

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
