#pragma once

/// @file formats/json/inline.hpp
/// @brief Inline value builders

#include <chrono>
#include <cstdint>
#include <string_view>

#include <formats/json/impl/types.hpp>
#include <formats/json/value.hpp>
#include <utils/fast_pimpl.hpp>

namespace formats::json {

/// Constructs an object Value from provided key-value pairs
template <typename... Args>
Value MakeObject(Args&&...);

/// Constructs an array Value from provided element list
template <typename... Args>
Value MakeArray(Args&&...);

namespace impl {

class InlineObjectBuilder {
 public:
  InlineObjectBuilder();

  template <typename... Args>
  formats::json::Value Build(Args&&... args) {
    static_assert(
        sizeof...(args) % 2 == 0,
        "Cannot build an object from an odd number of key-value arguments");
    Reserve(sizeof...(args) / 2);
    return DoBuild(std::forward<Args>(args)...);
  }

 private:
  formats::json::Value DoBuild();

  template <typename FieldValue, typename... Tail>
  formats::json::Value DoBuild(std::string_view key, FieldValue&& value,
                               Tail&&... tail) {
    Append(key, std::forward<FieldValue>(value));
    return DoBuild(std::forward<Tail>(tail)...);
  }

  void Reserve(size_t);

  void Append(std::string_view key, std::nullptr_t);
  void Append(std::string_view key, bool);
  void Append(std::string_view key, int32_t);
  void Append(std::string_view key, int64_t);
  void Append(std::string_view key, uint32_t);
  void Append(std::string_view key, uint64_t);
// MAC_COMPAT: different typedefs for 64_t on mac
#ifdef __APPLE__
  void Append(std::string_view key, long);
  void Append(std::string_view key, unsigned long);
#else
  void Append(std::string_view key, long long);
  void Append(std::string_view key, unsigned long long);
#endif
  void Append(std::string_view key, double);
  void Append(std::string_view key, const char*);
  void Append(std::string_view key, std::string_view);
  void Append(std::string_view key, std::chrono::system_clock::time_point);

  void Append(std::string_view key, const formats::json::Value&);

  VersionedValuePtr json_;
};

class InlineArrayBuilder {
 public:
  InlineArrayBuilder();

  formats::json::Value Build();

  template <typename... Elements>
  formats::json::Value Build(Elements&&... elements) {
    Reserve(sizeof...(elements));
    (Append(std::forward<Elements>(elements)), ...);
    return Build();
  }

 private:
  void Reserve(size_t);

  void Append(std::nullptr_t);
  void Append(bool);
  void Append(int32_t);
  void Append(int64_t);
  void Append(uint64_t);
// MAC_COMPAT: different typedefs for 64_t on mac
#ifdef __APPLE__
  void Append(long);
  void Append(unsigned long);
#else
  void Append(long long);
  void Append(unsigned long long);
#endif
  void Append(double);
  void Append(const char*);
  void Append(std::string_view);
  void Append(std::chrono::system_clock::time_point);

  void Append(const formats::json::Value&);

  VersionedValuePtr json_;
};

}  // namespace impl

/// @cond
template <typename... Args>
Value MakeObject(Args&&... args) {
  return impl::InlineObjectBuilder().Build(std::forward<Args>(args)...);
}

template <typename... Args>
Value MakeArray(Args&&... args) {
  return impl::InlineArrayBuilder().Build(std::forward<Args>(args)...);
}
/// @endcond

}  // namespace formats::json
