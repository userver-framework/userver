#pragma once

#include <userver/decimal64/decimal64.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::io {

template <typename T>
void RestoreCb(void* source, std::string_view db_representation) {
  auto* decimal = static_cast<T*>(source);
  UASSERT(decimal);

  *decimal = T{db_representation};
}

template <typename T>
std::string GetValueCb(void* source) {
  auto* decimal = static_cast<T*>(source);
  UASSERT(decimal);

  return ToString(*decimal);
}

class DecimalWrapper {
 public:
  DecimalWrapper();

  template <int Prec, typename Policy>
  DecimalWrapper(userver::decimal64::Decimal<Prec, Policy>& decimal);

  template <int Prec, typename Policy>
  DecimalWrapper(const userver::decimal64::Decimal<Prec, Policy>& decimal);

  std::string GetValue() const;

  void Restore(std::string_view db_representation) const;

 private:
  void* source_{nullptr};
  std::string (*get_value_cb_)(void*){nullptr};
  void (*restore_cb_)(void*, std::string_view){nullptr};
};

template <int Prec, typename Policy>
DecimalWrapper::DecimalWrapper(
    userver::decimal64::Decimal<Prec, Policy>& decimal)
    : source_{&decimal},
      restore_cb_{&RestoreCb<userver::decimal64::Decimal<Prec, Policy>>} {}

template <int Prec, typename Policy>
DecimalWrapper::DecimalWrapper(
    const userver::decimal64::Decimal<Prec, Policy>& decimal)
    // We only ever read from it via ToString()
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    : source_{const_cast<userver::decimal64::Decimal<Prec, Policy>*>(&decimal)},
      get_value_cb_{&GetValueCb<userver::decimal64::Decimal<Prec, Policy>>} {}

}  // namespace storages::mysql::io

USERVER_NAMESPACE_END
