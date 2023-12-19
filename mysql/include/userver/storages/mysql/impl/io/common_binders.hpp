#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <userver/formats/json_fwd.hpp>

#include <userver/storages/mysql/dates.hpp>
#include <userver/storages/mysql/impl/binder_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

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

// -------------------------- Output Bindings ---------------------------------
// yes, we do have to define optional<> variants separately,
// it's not that straightforward for output.
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::uint8_t> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::uint8_t>> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::int8_t> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::int8_t>> val);

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::uint16_t> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::uint16_t>> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::int16_t> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::int16_t>> val);

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::uint32_t> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::uint32_t>> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::int32_t> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::int32_t>> val);

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::uint64_t> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::uint64_t>> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::int64_t> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::int64_t>> val);

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<float> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<float>> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<double> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<double>> val);

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::string> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::string>> val);

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<formats::json::Value> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<formats::json::Value>> val);

// These 2 should never be called, so we just leave them unimplemented
void FreestandingBind(OutputBindingsFwd&, std::size_t,
                      ExplicitRef<std::string_view>);
void FreestandingBind(OutputBindingsFwd&, std::size_t,
                      ExplicitRef<std::optional<std::string_view>>);

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::chrono::system_clock::time_point> val);
void FreestandingBind(
    OutputBindingsFwd& binds, std::size_t pos,
    ExplicitRef<std::optional<std::chrono::system_clock::time_point>> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<Date> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<Date>> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<DateTime> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<DateTime>> val);

template <typename T>
void FreestandingBind(OutputBindingsFwd&, std::size_t, ExplicitRef<T>) {
  static_assert(!sizeof(T), "IO support for the type is not implemented.");
}

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
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<formats::json::Value> val);

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::chrono::system_clock::time_point> val);
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<Date> val);
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<DateTime> val);

void BindNull(InputBindingsFwd& binds, std::size_t pos);

template <typename T>
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::optional<T>> val) {
  if (val.Get().has_value()) {
    storages::mysql::impl::io::FreestandingBind(binds, pos,
                                                ExplicitCRef<T>{*val.Get()});
  } else {
    storages::mysql::impl::io::BindNull(binds, pos);
  }
}

template <typename T>
void FreestandingBind(InputBindingsFwd&, std::size_t, ExplicitCRef<T>) {
  static_assert(!sizeof(T), "IO support for the type is not implemented.");
}

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
