#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <boost/pfr/core.hpp>

#include <userver/utils/fast_pimpl.hpp>

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace impl {

namespace bindings {
class InputBindings;
class OutputBindings;
}  // namespace bindings

using InputBindingsPimpl = utils::FastPimpl<bindings::InputBindings, 72, 8>;
using OutputBindingsPimpl = utils::FastPimpl<bindings::OutputBindings, 80, 8>;
}  // namespace impl

namespace io {

template <typename BindsStorage, typename Field>
class BinderBase {
 public:
  BinderBase(BindsStorage& binds, std::size_t pos, Field& field)
      : binds_{binds}, pos_{pos}, field_{field} {}

  void operator()() { Bind(); }

 protected:
  virtual void Bind() = 0;

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  BindsStorage& binds_;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::size_t pos_;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  Field& field_;
};

template <typename T>
class InputBinder final {
 public:
  InputBinder(impl::bindings::InputBindings&, std::size_t, T&) {
    using SteadyClock = std::chrono::steady_clock;
    using Mutable = std::remove_const_t<T>;
    if constexpr (std::is_same_v<SteadyClock::time_point, Mutable> ||
                  std::is_same_v<std::optional<SteadyClock::time_point>,
                                 Mutable>) {
      static_assert(!sizeof(Mutable),
                    "Don't store steady_clock times in the database, use "
                    "system_clock instead");
    } else {
      static_assert(!sizeof(Mutable),
                    "Binding for the type is not implemented");
    }
  }
};

template <typename T>
class OutputBinder final {
 public:
  OutputBinder(impl::bindings::OutputBindings&, std::size_t, T&) {
    using SteadyClock = std::chrono::steady_clock;
    if constexpr (std::is_same_v<SteadyClock::time_point, T> ||
                  std::is_same_v<std::optional<SteadyClock::time_point>, T>) {
      static_assert(!sizeof(T),
                    "Don't store steady_clock times in the database, use "
                    "system_clock instead");
    } else if constexpr (std::is_same_v<std::string_view, T> ||
                         std::is_same_v<std::optional<std::string_view>, T>) {
      static_assert(
          !sizeof(T),
          "Don't use std::string_view in output params, since it's not-owning");
    } else {
      static_assert(!sizeof(T), "Binding for the type is not implemented");
    }
  }
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_BINDER_T(type)                                         \
  template <>                                                          \
  class InputBinder<const type> final                                  \
      : public BinderBase<impl::bindings::InputBindings, const type> { \
    using BinderBase::BinderBase;                                      \
    void Bind() final;                                                 \
  };                                                                   \
                                                                       \
  template <>                                                          \
  class OutputBinder<type> final                                       \
      : public BinderBase<impl::bindings::OutputBindings, type> {      \
    using BinderBase::BinderBase;                                      \
    void Bind() final;                                                 \
  };

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_BINDER_OPTT(type)                         \
  template <>                                             \
  class InputBinder<const std::optional<type>> final      \
      : public BinderBase<impl::bindings::InputBindings,  \
                          const std::optional<type>> {    \
    using BinderBase::BinderBase;                         \
    void Bind() final;                                    \
  };                                                      \
                                                          \
  template <>                                             \
  class OutputBinder<std::optional<type>> final           \
      : public BinderBase<impl::bindings::OutputBindings, \
                          std::optional<type>> {          \
    using BinderBase::BinderBase;                         \
    void Bind() final;                                    \
  };

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_BINDER(type) \
  DECLARE_BINDER_T(type)     \
  DECLARE_BINDER_OPTT(type)

// numeric types
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
// TODO : decimal
// string types
DECLARE_BINDER(std::string)
DECLARE_BINDER(std::string_view)  // why not for input, disabled for output
DECLARE_BINDER(formats::json::Value)
// date types
DECLARE_BINDER(std::chrono::system_clock::time_point)
// TODO : duration

template <typename T>
auto GetInputBinder(impl::bindings::InputBindings& binds, std::size_t pos,
                    T& field) {
  return InputBinder<T>{binds, pos, field};
  // return GetBinder<InputBinder>(binds, pos, field);
}

template <typename T>
auto GetOutputBinder(impl::bindings::OutputBindings& binds, std::size_t pos,
                     T& field) {
  return OutputBinder<T>(binds, pos, field);
}

class ParamsBinderBase {
 public:
  virtual impl::bindings::InputBindings& GetBinds() = 0;

  virtual std::size_t GetRowsCount() const { return 0; }
};

class ParamsBinder final : public ParamsBinderBase {
 public:
  explicit ParamsBinder(std::size_t size);
  ~ParamsBinder();

  ParamsBinder(const ParamsBinder& other) = delete;
  ParamsBinder(ParamsBinder&& other) noexcept;

  impl::bindings::InputBindings& GetBinds() final;

  template <typename Field>
  void Bind(std::size_t pos, const Field& field) {
    GetInputBinder(GetBinds(), pos, field)();
  }

  std::size_t GetRowsCount() const final { return 1; }

  template <typename... Args>
  static ParamsBinder BindParams(const Args&... args) {
    constexpr auto kParamsCount = sizeof...(Args);
    ParamsBinder binder{kParamsCount};

    if constexpr (kParamsCount > 0) {
      size_t ind = 0;
      const auto do_bind = [&binder](std::size_t pos, const auto& arg) {
        // TODO : this catches too much probably
        if constexpr (std::is_convertible_v<decltype(arg), std::string_view>) {
          std::string_view sw{arg};
          binder.Bind(pos, sw);
        } else {
          binder.Bind(pos, arg);
        }
      };

      (..., do_bind(ind++, args));
    }

    return binder;
  }

 private:
  impl::InputBindingsPimpl impl_;
};

class ResultBinder final {
 public:
  explicit ResultBinder(std::size_t size);
  ~ResultBinder();

  ResultBinder(const ResultBinder& other) = delete;
  ResultBinder(ResultBinder&& other) noexcept;

  template <typename T>
  impl::bindings::OutputBindings& BindTo(T& row) {
    boost::pfr::for_each_field(row, FieldBinder{*impl_});

    return GetBinds();
  }

  impl::bindings::OutputBindings& GetBinds();

 private:
  class FieldBinder final {
   public:
    explicit FieldBinder(impl::bindings::OutputBindings& binds)
        : binds_{binds} {}

    template <typename Field, size_t Index>
    void operator()(Field& field,
                    std::integral_constant<std::size_t, Index> i) {
      GetOutputBinder(binds_, i, field)();
    }

   private:
    impl::bindings::OutputBindings& binds_;
  };

  impl::OutputBindingsPimpl impl_;
};

}  // namespace io

}  // namespace storages::mysql

USERVER_NAMESPACE_END
