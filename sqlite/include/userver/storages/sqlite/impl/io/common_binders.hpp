#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>

#include <userver/storages/sqlite/impl/binder_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl::io {

template <typename T>
class ExplicitRef final {
 public:
  explicit ExplicitRef(T& ref) : ref_{ref} {}
  explicit ExplicitRef(T&& ref) = delete;

  T& Get() { return ref_; }

 private:
  T& ref_;
};

template <typename T>
class ExplicitCRef final {
 public:
  static_assert(!std::is_const_v<T>);

  explicit ExplicitCRef(const T& ref) : ref_{ref} {}
  explicit ExplicitCRef(T&& ref) = delete;

  const T& Get() const { return ref_; }

 private:
  const T& ref_;
};

// --------------------------- Input Bindings ---------------------------------
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::uint8_t> val);
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::int8_t> val);

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::uint16_t> val);
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::int16_t> val);

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::uint32_t> val);
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::int32_t> val);

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::uint64_t> val);
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::int64_t> val);

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<float> val);
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<double> val);

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::string> val);
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::string_view> val);

void BindNull(InputBindingsFwd& binds, std::size_t pos);

template <typename T>
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::optional<T>> val) {
  if (val.Get().has_value()) {
    storages::sqlite::impl::io::FreestandingBind(binds, pos,
                                                 ExplicitCRef<T>{*val.Get()});
  } else {
    storages::sqlite::impl::io::BindNull(binds, pos);
  }
}

template <typename T>
void FreestandingBind(InputBindingsFwd&, std::size_t, ExplicitCRef<T>) {
  static_assert(!sizeof(T), "IO support for the type is not implemented.");
}

}  // namespace storages::sqlite::impl::io

USERVER_NAMESPACE_END
