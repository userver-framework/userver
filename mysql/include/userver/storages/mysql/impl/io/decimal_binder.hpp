#pragma once

#include <userver/decimal64/decimal64.hpp>

#include <userver/storages/mysql/impl/binder_fwd.hpp>
#include <userver/storages/mysql/impl/io/common_binders.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

template <int Prec, typename Policy>
using Decimal64 = decimal64::Decimal<Prec, Policy>;

template <int Prec, typename Policy>
using Decimal64Opt = std::optional<Decimal64<Prec, Policy>>;

class DecimalWrapper {
 public:
  // For input
  template <int Prec, typename Policy>
  DecimalWrapper(const Decimal64<Prec, Policy>& decimal);

  // For output
  template <int Prec, typename Policy>
  DecimalWrapper(Decimal64<Prec, Policy>& decimal);

  // For optional output
  template <int Prec, typename Policy>
  DecimalWrapper(std::optional<Decimal64<Prec, Policy>>& opt_decimal);

 private:
  friend class storages::mysql::impl::bindings::OutputBindings;
  friend class storages::mysql::impl::bindings::InputBindings;

  DecimalWrapper();

  std::string GetValue() const;

  void Restore(std::string_view db_representation) const;

  template <typename T>
  static void RestoreCb(void* source, std::string_view db_representation) {
    auto* decimal = static_cast<T*>(source);
    UASSERT(decimal);

    *decimal = T{db_representation};
  }

  template <typename T>
  static void RestoreOptionalCb(void* source,
                                std::string_view db_representation) {
    auto* optional = static_cast<std::optional<T>*>(source);
    UASSERT(optional);

    optional->emplace(db_representation);
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
DecimalWrapper::DecimalWrapper(const Decimal64<Prec, Policy>& decimal)
    // We only ever read from it via ToString()
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    : source_{const_cast<Decimal64<Prec, Policy>*>(&decimal)},
      // this constructor takes const reference, that means we are binding
      // for input
      get_value_cb_{GetValueCb<Decimal64<Prec, Policy>>} {}

template <int Prec, typename Policy>
DecimalWrapper::DecimalWrapper(Decimal64<Prec, Policy>& decimal)
    : source_{&decimal},
      // this constructor takes non-const reference, that means we are binding
      // for output
      restore_cb_{RestoreCb<Decimal64<Prec, Policy>>} {}

template <int Prec, typename Policy>
DecimalWrapper::DecimalWrapper(
    std::optional<Decimal64<Prec, Policy>>& opt_decimal)
    : source_{&opt_decimal},
      // this constructor takes non-const reference, that means we are binding
      // for output optional
      restore_cb_{RestoreOptionalCb<Decimal64<Prec, Policy>>} {}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      io::DecimalWrapper& val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      std::optional<io::DecimalWrapper>& val);

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      const io::DecimalWrapper& val);
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      const std::optional<io::DecimalWrapper>& val);

template <int Prec, typename Policy>
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<Decimal64<Prec, Policy>> field) {
  DecimalWrapper wrapper{field.Get()};

  storages::mysql::impl::io::FreestandingBind(binds, pos, wrapper);
}

template <int Prec, typename Policy>
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<Decimal64Opt<Prec, Policy>> field) {
  const auto wrapper = [&field]() -> std::optional<DecimalWrapper> {
    if (field.Get().has_value()) {
      return DecimalWrapper{*field.Get()};
    } else {
      return std::nullopt;
    }
  }();

  storages::mysql::impl::io::FreestandingBind(binds, pos, wrapper);
}

template <int Prec, typename Policy>
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<Decimal64<Prec, Policy>> field) {
  DecimalWrapper wrapper{field.Get()};

  storages::mysql::impl::io::FreestandingBind(binds, pos, wrapper);
}

template <int Prec, typename Policy>
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<Decimal64Opt<Prec, Policy>> field) {
  // This doesn't have to be optional, but it's easier to distinguish this way
  std::optional<DecimalWrapper> wrapper{field.Get()};

  storages::mysql::impl::io::FreestandingBind(binds, pos, wrapper);
}

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
