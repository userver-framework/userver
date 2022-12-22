#pragma once

#include <userver/decimal64/decimal64.hpp>

#include <userver/storages/mysql/impl/binder_fwd.hpp>
#include <userver/storages/mysql/impl/io/base_binder.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

template <int Prec, typename Policy>
using Decimal64 = userver::decimal64::Decimal<Prec, Policy>;

template <int Prec, typename Policy>
using Decimal64Opt = std::optional<Decimal64<Prec, Policy>>;

class DecimalWrapper {
 public:
  template <int Prec, typename Policy>
  DecimalWrapper(Decimal64<Prec, Policy>& decimal);

  template <int Prec, typename Policy>
  DecimalWrapper(const Decimal64<Prec, Policy>& decimal);

  std::string GetValue() const;

  void Restore(std::string_view db_representation) const;

 private:
  friend class storages::mysql::impl::bindings::OutputBindings;
  DecimalWrapper();

  template <typename T>
  static void RestoreCb(void* source, std::string_view db_representation) {
    auto* decimal = static_cast<T*>(source);
    UASSERT(decimal);

    *decimal = T{db_representation};
  }

  template <typename T>
  static std::string GetValueCb(void* source) {
    auto* decimal = static_cast<T*>(source);
    UASSERT(decimal);

    return ToString(*decimal);
  }

  void* source_{nullptr};
  std::string (*get_value_cb_)(void*){nullptr};
  void (*restore_cb_)(void*, std::string_view){nullptr};
};

template <int Prec, typename Policy>
DecimalWrapper::DecimalWrapper(Decimal64<Prec, Policy>& decimal)
    : source_{&decimal},
      // this constructor take non-const reference, that means we are binding
      // for output
      restore_cb_{&RestoreCb<Decimal64<Prec, Policy>>} {}

template <int Prec, typename Policy>
DecimalWrapper::DecimalWrapper(const Decimal64<Prec, Policy>& decimal)
    // We only ever read from it via ToString()
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    : source_{const_cast<Decimal64<Prec, Policy>*>(&decimal)},
      // this constructor take const reference, that means we are binding
      // for input
      get_value_cb_{&GetValueCb<Decimal64<Prec, Policy>>} {}

template <>
class InputBinder<DecimalWrapper> final
    : public InputBinderBase<DecimalWrapper> {
  using InputBinderBase<DecimalWrapper>::InputBinderBase;
  void Bind() final;
};
template <>
class InputBinder<std::optional<DecimalWrapper>> final
    : public InputBinderBase<std::optional<DecimalWrapper>> {
  using InputBinderBase<std::optional<DecimalWrapper>>::InputBinderBase;
  void Bind() final;
};
template <>
class OutputBinder<DecimalWrapper> final
    : public OutputBinderBase<DecimalWrapper> {
  using OutputBinderBase<DecimalWrapper>::OutputBinderBase;
  void Bind() final;
};
template <>
class OutputBinder<std::optional<DecimalWrapper>> final
    : public OutputBinderBase<std::optional<DecimalWrapper>> {
  using OutputBinderBase<std::optional<DecimalWrapper>>::OutputBinderBase;
  void Bind() final;
};

template <int Prec, typename Policy>
class InputBinder<Decimal64<Prec, Policy>> final
    : public InputBinderBase<Decimal64<Prec, Policy>> {
 public:
  static constexpr bool kIsSupported = true;
  using InputBinderBase<Decimal64<Prec, Policy>>::InputBinderBase;

  void Bind() final {
    DecimalWrapper wrapper{this->field_};
    InputBinder<DecimalWrapper>{this->binds_, this->pos_, wrapper}();
  }
};

template <int Prec, typename Policy>
class InputBinder<Decimal64Opt<Prec, Policy>> final
    : public InputBinderBase<Decimal64Opt<Prec, Policy>> {
 public:
  static constexpr bool kIsSupported = true;
  using InputBinderBase<Decimal64Opt<Prec, Policy>>::InputBinderBase;

  void Bind() final {
    const auto wrapper = [this]() -> std::optional<DecimalWrapper> {
      if (this->field_.has_value()) {
        return DecimalWrapper{*this->field_};
      } else {
        return std::nullopt;
      }
    }();
    InputBinder<std::optional<DecimalWrapper>>{this->binds_, this->pos_,
                                               wrapper}();
  }
};

template <int Prec, typename Policy>
class OutputBinder<Decimal64<Prec, Policy>> final
    : public OutputBinderBase<Decimal64<Prec, Policy>> {
 public:
  static constexpr bool kIsSupported = true;
  using OutputBinderBase<Decimal64<Prec, Policy>>::OutputBinderBase;

  void Bind() final {
    DecimalWrapper wrapper{this->field_};
    OutputBinder<DecimalWrapper>{this->binds_, this->pos_, wrapper}();
  }
};

template <int Prec, typename Policy>
class OutputBinder<Decimal64Opt<Prec, Policy>> final
    : public OutputBinderBase<Decimal64Opt<Prec, Policy>> {
 public:
  static constexpr bool kIsSupported = true;
  using OutputBinderBase<Decimal64Opt<Prec, Policy>>::OutputBinderBase;

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

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
