#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <storages/mysql/impl/mariadb_include.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::bindings {

template <bool Const>
class BindsStorageInterface {
 public:
  virtual std::size_t Size() const = 0;
  virtual bool Empty() const = 0;
  virtual MYSQL_BIND* GetBindsArray() = 0;

  template <typename T>
  using O = std::optional<T>;

  template <typename T>
  using C = std::conditional_t<Const, const T, T>;

  virtual void Bind(std::size_t pos, C<std::uint8_t>& val) = 0;
  virtual void Bind(std::size_t pos, C<O<std::uint8_t>>& val) = 0;
  virtual void Bind(std::size_t pos, C<std::int8_t>& val) = 0;
  virtual void Bind(std::size_t pos, C<O<std::int8_t>>& val) = 0;

  virtual void Bind(std::size_t pos, C<std::uint16_t>& val) = 0;
  virtual void Bind(std::size_t pos, C<O<std::uint16_t>>& val) = 0;
  virtual void Bind(std::size_t pos, C<std::int16_t>& val) = 0;
  virtual void Bind(std::size_t pos, C<O<std::int16_t>>& val) = 0;

  virtual void Bind(std::size_t pos, C<std::uint32_t>& val) = 0;
  virtual void Bind(std::size_t pos, C<O<std::uint32_t>>& val) = 0;
  virtual void Bind(std::size_t pos, C<std::int32_t>& val) = 0;
  virtual void Bind(std::size_t pos, C<O<std::int32_t>>& val) = 0;

  virtual void Bind(std::size_t pos, C<std::uint64_t>& val) = 0;
  virtual void Bind(std::size_t pos, C<O<std::uint64_t>>& val) = 0;
  virtual void Bind(std::size_t pos, C<std::int64_t>& val) = 0;
  virtual void Bind(std::size_t pos, C<O<std::int64_t>>& val) = 0;

  virtual void Bind(std::size_t pos, C<float>& val) = 0;
  virtual void Bind(std::size_t pos, C<O<float>>& val) = 0;
  virtual void Bind(std::size_t pos, C<double>& val) = 0;
  virtual void Bind(std::size_t pos, C<O<double>>& val) = 0;

  virtual void Bind(std::size_t pos, C<std::string>& val) = 0;
  virtual void Bind(std::size_t pos, C<O<std::string>>& val) = 0;
  virtual void Bind(std::size_t pos, C<std::string_view>& val) = 0;
  virtual void Bind(std::size_t pos, C<O<std::string_view>>& val) = 0;

  virtual void Bind(std::size_t pos,
                    C<std::chrono::system_clock::time_point>& val) = 0;
  virtual void Bind(std::size_t pos,
                    C<O<std::chrono::system_clock::time_point>>& val) = 0;

  template <typename T>
  static constexpr enum_field_types GetNativeType();

  template <typename T>
  static constexpr enum_field_types GetNativeType(T&);

 protected:
  using MariaDBTimepointResolution = std::chrono::microseconds;

  ~BindsStorageInterface() = default;
};

template <bool Const>
template <typename U>
constexpr enum_field_types BindsStorageInterface<Const>::GetNativeType() {
  using T = std::remove_const_t<U>;

  if constexpr (std::is_same_v<std::uint8_t, T> ||
                std::is_same_v<std::int8_t, T>) {
    return MYSQL_TYPE_TINY;
  } else if constexpr (std::is_same_v<std::uint16_t, T> ||
                       std::is_same_v<std::int16_t, T>) {
    return MYSQL_TYPE_SHORT;
  } else if constexpr (std::is_same_v<std::uint32_t, T> ||
                       std::is_same_v<std::int32_t, T>) {
    return MYSQL_TYPE_LONG;
  } else if constexpr (std::is_same_v<std::uint64_t, T> ||
                       std::is_same_v<std::int64_t, T>) {
    return MYSQL_TYPE_LONGLONG;
  } else if constexpr (std::is_same_v<float, T>) {
    return MYSQL_TYPE_FLOAT;
  } else if constexpr (std::is_same_v<double, T>) {
    return MYSQL_TYPE_DOUBLE;
  } else if constexpr (std::is_same_v<std::string, T> ||
                       std::is_same_v<std::string_view, T>) {
    return MYSQL_TYPE_STRING;
  } else if constexpr (std::is_same_v<std::chrono::system_clock::time_point,
                                      T>) {
    return MYSQL_TYPE_DATETIME;
  } else {
    static_assert(!sizeof(T), "This shouldn't fire, fix me");
  }
}

template <bool Const>
template <typename T>
constexpr enum_field_types BindsStorageInterface<Const>::GetNativeType(T&) {
  return GetNativeType<T>();
}

}  // namespace storages::mysql::impl::bindings

USERVER_NAMESPACE_END
