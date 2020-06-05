#pragma once

#include <map>
#include <unordered_map>
#include <variant>
#include <vector>

#include <formats/json/value.hpp>
#include <formats/json/value_builder.hpp>
#include <formats/serialize/to.hpp>
#include <utils/fast_pimpl.hpp>
#include <utils/meta.hpp>

namespace formats::json::impl {

class StringBuilder final {
 public:
  StringBuilder();
  ~StringBuilder();

  class ObjectGuard final {
   public:
    explicit ObjectGuard(StringBuilder& sw);
    ~ObjectGuard();

   private:
    StringBuilder& sw_;
  };

  class ArrayGuard final {
   public:
    explicit ArrayGuard(StringBuilder& sw);
    ~ArrayGuard();

   private:
    StringBuilder& sw_;
  };

  std::string GetString() const;

  void WriteNull();
  void WriteString(std::string_view value);
  void WriteBool(bool value);
  void WriteInt64(int64_t value);
  void WriteUInt64(uint64_t value);
  void WriteDouble(double value);

  // For object only
  void Key(std::string_view sw);

  void WriteRawString(std::string_view value);

  void WriteValue(const ::formats::json::Value& value);

 private:
  struct Impl;
  utils::FastPimpl<Impl, 112, 8> impl_;
};

void WriteToStream(bool value, StringBuilder& sw);
void WriteToStream(int64_t value, StringBuilder& sw);
void WriteToStream(uint64_t value, StringBuilder& sw);
void WriteToStream(double value, StringBuilder& sw);
void WriteToStream(std::string_view value, StringBuilder& sw);
void WriteToStream(const formats::json::Value& value, StringBuilder& sw);

void WriteToStream(const std::string& value, StringBuilder& sw);

template <typename T>
std::enable_if_t<meta::is_vector<T>::value || meta::is_array<T>::value ||
                     meta::is_set<T>::value,
                 void>
WriteToStream(const T& value, StringBuilder& sw) {
  StringBuilder::ArrayGuard guard(sw);
  for (const auto& item : value) WriteToStream(item, sw);
}

template <typename T>
std::enable_if_t<meta::is_map<T>::value, void> WriteToStream(
    const T& value, StringBuilder& sw) {
  StringBuilder::ObjectGuard guard(sw);
  for (const auto& [key, value] : value) {
    sw.Key(key);
    WriteToStream(value, sw);
  }
}

template <typename T>
std::enable_if_t<formats::common::kHasSerializeTo<::formats::json::Value, T> &&
                     !meta::is_vector<T>::value && !meta::is_set<T>::value &&
                     !meta::is_map<T>::value,
                 void>
WriteToStream(const T& value, StringBuilder& sw) {
  sw.WriteValue(Serialize(value, serialize::To<::formats::json::Value>{}));
}

template <typename... Types>
void WriteToStream(const std::variant<Types...>& value, StringBuilder& sw) {
  return std::visit([&sw](const auto& item) { WriteToStream(item, sw); },
                    value);
}

}  // namespace formats::json::impl
