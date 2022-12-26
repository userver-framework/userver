#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include <userver/formats/json_fwd.hpp>
#include <userver/storages/mysql/dates.hpp>
#include <userver/storages/mysql/impl/io/base_binder.hpp>
#include <userver/storages/mysql/impl/io/decimal_binder.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

// Tidy gets upset with <type> and insists on parentheses around 'type',
// which is invalid C++.
// Tidy also heavily dislikes macro usage, but these are just a bunch of codegen
// macros, so idc.
// NOLINTBEGIN
#define DECLARE_INPUT_BINDER(type)                               \
  template <>                                                    \
  class InputBinder<type> final : public InputBinderBase<type> { \
   public:                                                       \
    static constexpr bool kIsSupported = true;                   \
    using InputBinderBase<type>::InputBinderBase;                \
    void Bind() final;                                           \
  };                                                             \
  template <>                                                    \
  class InputBinder<std::optional<type>> final                   \
      : public InputBinderBase<std::optional<type>> {            \
   public:                                                       \
    static constexpr bool kIsSupported = true;                   \
    using InputBinderBase<std::optional<type>>::InputBinderBase; \
    void Bind() final;                                           \
  };

#define DECLARE_OUTPUT_BINDER(type)                                \
  template <>                                                      \
  class OutputBinder<type> final : public OutputBinderBase<type> { \
   public:                                                         \
    static constexpr bool kIsSupported = true;                     \
    using OutputBinderBase<type>::OutputBinderBase;                \
    void Bind() final;                                             \
  };                                                               \
  template <>                                                      \
  class OutputBinder<std::optional<type>> final                    \
      : public OutputBinderBase<std::optional<type>> {             \
   public:                                                         \
    static constexpr bool kIsSupported = true;                     \
    using OutputBinderBase<std::optional<type>>::OutputBinderBase; \
    void Bind() final;                                             \
  };

#define DECLARE_BINDER(type) \
  DECLARE_INPUT_BINDER(type) \
  DECLARE_OUTPUT_BINDER(type)
// NOLINTEND

// numeric types
// TODO : bool
DECLARE_BINDER(std::uint8_t)
DECLARE_BINDER(std::int8_t)
DECLARE_BINDER(std::uint16_t)
DECLARE_BINDER(std::int16_t)
DECLARE_BINDER(std::uint32_t)
DECLARE_BINDER(std::int32_t)
DECLARE_BINDER(std::uint64_t)
DECLARE_BINDER(std::int64_t)
DECLARE_BINDER(float)
DECLARE_BINDER(double)
// Binders for decimal are specialized in decimal_binder.hpp
// string types
// string-alike types
DECLARE_BINDER(std::string)
DECLARE_BINDER(std::string_view)  // why not for input, disabled for output
DECLARE_BINDER(formats::json::Value)
// date types
DECLARE_BINDER(std::chrono::system_clock::time_point)
DECLARE_BINDER(storages::mysql::Date)
DECLARE_BINDER(storages::mysql::DateTime)
// TODO : duration

#undef DECLARE_INPUT_BINDER
#undef DECLARE_OUTPUT_BINDER
#undef DECLARE_BINDER

template <typename T>
auto GetInputBinder(mysql::impl::InputBindingsFwd& binds, std::size_t pos,
                    const T& field) {
  return InputBinder<T>{binds, pos, field};
}

template <typename T>
auto GetOutputBinder(mysql::impl::OutputBindingsFwd& binds, std::size_t pos,
                     T& field) {
  if constexpr (std::is_same_v<std::string_view, T> ||
                std::is_same_v<std::optional<std::string_view>, T>) {
    static_assert(
        !sizeof(T),
        "Don't use std::string_view in output params, since it's not-owning");
  }

  return OutputBinder<T>{binds, pos, field};
}

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
