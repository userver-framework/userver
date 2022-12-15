#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <boost/pfr/core.hpp>

#include <userver/storages/mysql/io/decimal_wrapper.hpp>

#include <userver/utils/fast_pimpl.hpp>

#include <userver/decimal64/decimal64.hpp>
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

using InputBindingsFwd = impl::bindings::InputBindings;
using OutputBindingsFwd = impl::bindings::OutputBindings;

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
class InputBinder final : public BinderBase<InputBindingsFwd, T> {
 public:
  using BinderBase<InputBindingsFwd, T>::BinderBase;

  void Bind() final {
    using SteadyClock = std::chrono::steady_clock;
    using Mutable = std::decay_t<T>;

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
class OutputBinder final : public BinderBase<OutputBindingsFwd, T> {
 public:
  using BinderBase<OutputBindingsFwd, T>::BinderBase;

  void Bind() final {
    using SteadyClock = std::chrono::steady_clock;
    if constexpr (std::is_same_v<SteadyClock::time_point, T> ||
                  std::is_same_v<std::optional<SteadyClock::time_point>, T>) {
      static_assert(!sizeof(T),
                    "Don't store steady_clock times in the database, use "
                    "system_clock instead");
    } else {
      static_assert(!sizeof(T), "Binding for the type is not implemented");
    }
  }
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_BINDER_T(type)                            \
  template <>                                             \
  class InputBinder<const type> final                     \
      : public BinderBase<InputBindingsFwd, const type> { \
    using BinderBase::BinderBase;                         \
    void Bind() final;                                    \
  };                                                      \
                                                          \
  template <>                                             \
  class OutputBinder<type> final                          \
      : public BinderBase<OutputBindingsFwd, type> {      \
    using BinderBase::BinderBase;                         \
    void Bind() final;                                    \
  };

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_BINDER_OPTT(type)                                        \
  template <>                                                            \
  class InputBinder<const std::optional<type>> final                     \
      : public BinderBase<InputBindingsFwd, const std::optional<type>> { \
    using BinderBase::BinderBase;                                        \
    void Bind() final;                                                   \
  };                                                                     \
                                                                         \
  template <>                                                            \
  class OutputBinder<std::optional<type>> final                          \
      : public BinderBase<OutputBindingsFwd, std::optional<type>> {      \
    using BinderBase::BinderBase;                                        \
    void Bind() final;                                                   \
  };

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_BINDER(type) \
  DECLARE_BINDER_T(type)     \
  DECLARE_BINDER_OPTT(type)

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
DECLARE_BINDER(DecimalWrapper)
// TODO : decimal
// string types
DECLARE_BINDER(std::string)
DECLARE_BINDER(std::string_view)  // why not for input, disabled for output
DECLARE_BINDER(formats::json::Value)
// date types
DECLARE_BINDER(std::chrono::system_clock::time_point)
// TODO : duration

template <int Prec, typename Policy>
using Decimal64 = userver::decimal64::Decimal<Prec, Policy>;

template <int Prec, typename Policy>
using Decimal64Opt = std::optional<Decimal64<Prec, Policy>>;

template <int Prec, typename Policy>
class InputBinder<const Decimal64<Prec, Policy>> final
    : public BinderBase<InputBindingsFwd, const Decimal64<Prec, Policy>> {
 public:
  InputBinder(InputBindingsFwd& binds, std::size_t pos,
              const Decimal64<Prec, Policy>& field)
      : BinderBase<InputBindingsFwd, const Decimal64<Prec, Policy>>{binds, pos,
                                                                    field} {}

  void Bind() final {
    DecimalWrapper wrapper{this->field_};
    InputBinder<const DecimalWrapper>{this->binds_, this->pos_, wrapper}();
  }
};

template <int Prec, typename Policy>
class InputBinder<const Decimal64Opt<Prec, Policy>> final
    : public BinderBase<InputBindingsFwd, const Decimal64Opt<Prec, Policy>> {
 public:
  InputBinder(InputBindingsFwd& binds, std::size_t pos,
              const Decimal64Opt<Prec, Policy>& field)
      : BinderBase<InputBindingsFwd, const Decimal64Opt<Prec, Policy>>{
            binds, pos, field} {}

  void Bind() final {
    const auto wrapper = [this]() -> std::optional<DecimalWrapper> {
      if (this->field_.has_value()) {
        return DecimalWrapper{*this->field_};
      } else {
        return std::nullopt;
      }
    }();
    InputBinder<const std::optional<DecimalWrapper>>{this->binds_, this->pos_,
                                                     wrapper}();
  }
};

template <int Prec, typename Policy>
class OutputBinder<Decimal64<Prec, Policy>> final
    : public BinderBase<OutputBindingsFwd, Decimal64<Prec, Policy>> {
 public:
  using BinderBase<OutputBindingsFwd, Decimal64<Prec, Policy>>::BinderBase;

  void Bind() final {
    DecimalWrapper wrapper{this->field_};
    OutputBinder<DecimalWrapper>{this->binds_, this->pos_, wrapper}();
  }
};

template <int Prec, typename Policy>
class OutputBinder<Decimal64Opt<Prec, Policy>> final
    : public BinderBase<OutputBindingsFwd, Decimal64Opt<Prec, Policy>> {
 public:
  OutputBinder(OutputBindingsFwd& binds, std::size_t pos,
               Decimal64Opt<Prec, Policy>& field)
      : BinderBase<OutputBindingsFwd, Decimal64Opt<Prec, Policy>>{binds, pos,
                                                                  field} {}

  void Bind() final {
    auto wrapper = [this]() -> std::optional<DecimalWrapper> {
      if (this->field_.has_value()) {
        return DecimalWrapper{*this->field_};
      } else {
        return std::nullopt;
      }
    }();
    OutputBinder<std::optional<DecimalWrapper>>{this->binds_, this->pos_,
                                                wrapper}();
  }
};

template <typename T>
auto GetInputBinder(InputBindingsFwd& binds, std::size_t pos, T& field) {
  return InputBinder<T>{binds, pos, field};
}

template <typename T>
auto GetOutputBinder(OutputBindingsFwd& binds, std::size_t pos, T& field) {
  if constexpr (std::is_same_v<std::string_view, T> ||
                std::is_same_v<std::optional<std::string_view>, T>) {
    static_assert(
        !sizeof(T),
        "Don't use std::string_view in output params, since it's not-owning");
  }

  return OutputBinder<T>{binds, pos, field};
}

class ParamsBinderBase {
 public:
  virtual InputBindingsFwd& GetBinds() = 0;

  virtual std::size_t GetRowsCount() const { return 0; }
};

class ParamsBinder final : public ParamsBinderBase {
 public:
  explicit ParamsBinder(std::size_t size);
  ~ParamsBinder();

  ParamsBinder(const ParamsBinder& other) = delete;
  ParamsBinder(ParamsBinder&& other) noexcept;

  InputBindingsFwd& GetBinds() final;

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
        // TODO : this catches too much probably,
        // right now it's a workaround for `const char*`
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
  OutputBindingsFwd& BindTo(T& row) {
    boost::pfr::for_each_field(row, FieldBinder{*impl_});

    return GetBinds();
  }

  OutputBindingsFwd& GetBinds();

 private:
  class FieldBinder final {
   public:
    explicit FieldBinder(OutputBindingsFwd& binds) : binds_{binds} {}

    template <typename Field, size_t Index>
    void operator()(Field& field,
                    std::integral_constant<std::size_t, Index> i) {
      GetOutputBinder(binds_, i, field)();
    }

   private:
    OutputBindingsFwd& binds_;
  };

  impl::OutputBindingsPimpl impl_;
};

}  // namespace io

}  // namespace storages::mysql

USERVER_NAMESPACE_END
