#pragma once

/// @file userver/formats/bson/types.hpp
/// @brief BSON-specific types

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <bson/bson.h>

#include <userver/formats/common/path.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson {
namespace impl {
using BsonHolder = std::shared_ptr<const bson_t>;

class ValueImpl;
using ValueImplPtr = std::shared_ptr<ValueImpl>;

using ParsedArray = std::vector<ValueImplPtr>;
using ParsedDocument = std::unordered_map<std::string, ValueImplPtr>;
}  // namespace impl

using formats::common::Path;

inline constexpr auto kNull = nullptr;

/// BSON ObjectId
class Oid {
 public:
  /// Generates a new id
  Oid();

  /// Constructor from hex-encoded form, zero terminator is not required
  explicit Oid(std::string_view hex_encoded);

  /// Creates a minimal Oid with specified time point up to a second precision
  ///
  /// @throws formats::bson::BsonException if `time` is too big to be stored in
  /// ObjectId without narrowing.
  static Oid MakeMinimalFor(std::chrono::system_clock::time_point time);

  /// @cond
  /// Constructor from native type
  /* implicit */ Oid(const bson_oid_t&);
  /// @endcond

  /// Returns hex-encoded value
  std::string ToString() const;

  /// @name Raw value access
  /// @{
  const uint8_t* Data() const;

  constexpr static size_t Size() { return 12; }
  /// @}

  /// Returns stored unix timestamp
  time_t GetTimestamp() const;

  /// Returns stored time point
  std::chrono::system_clock::time_point GetTimePoint() const;

  /// @cond
  /// Native type access, internal use only
  const bson_oid_t* GetNative() const;
  /// @endcond

  bool operator==(const Oid&) const;
  bool operator!=(const Oid&) const;
  bool operator<(const Oid&) const;
  bool operator>(const Oid&) const;
  bool operator<=(const Oid&) const;
  bool operator>=(const Oid&) const;

 private:
  friend struct std::hash<Oid>;

  bson_oid_t oid_;
};

/// BSON Binary
class Binary {
 public:
  /// Constructor from a string storage
  explicit Binary(std::string data) : data_(std::move(data)) {}

  /// @name Raw data access
  /// @{
  const std::string& ToString() const& { return data_; }
  std::string&& ToString() && { return std::move(data_); }

  const uint8_t* Data() const {
    return reinterpret_cast<const uint8_t*>(data_.data());
  }
  size_t Size() const { return data_.size(); }
  /// @}

  bool operator==(const Binary& rhs) const { return data_ == rhs.data_; }
  bool operator!=(const Binary& rhs) const { return data_ != rhs.data_; }
  bool operator<(const Binary& rhs) const { return data_ < rhs.data_; }
  bool operator>(const Binary& rhs) const { return data_ > rhs.data_; }
  bool operator<=(const Binary& rhs) const { return data_ <= rhs.data_; }
  bool operator>=(const Binary& rhs) const { return data_ >= rhs.data_; }

 private:
  friend struct std::hash<Binary>;

  std::string data_;
};

/// @brief BSON Decimal128
/// @see
/// https://github.com/mongodb/specifications/blob/master/source/bson-decimal128/decimal128.rst
class Decimal128 {
 public:
  /// Constructor from a string form
  explicit Decimal128(const std::string& value);

  /// @cond
  /// Constructor from native type
  /* implicit */ Decimal128(const bson_decimal128_t&);
  /// @endcond

  /// Returns string form
  std::string ToString() const;

  /// Returns an infinite value
  static Decimal128 Infinity();

  /// Returns a not-a-number value
  static Decimal128 NaN();

  /// @cond
  /// Native type access, internal use only
  const bson_decimal128_t* GetNative() const;
  /// @endcond

  bool operator==(const Decimal128&) const;
  bool operator!=(const Decimal128&) const;

 private:
  bson_decimal128_t decimal_;
};

/// BSON MinKey
class MinKey {};

/// BSON MaxKey
class MaxKey {};

/// @brief BSON Timestamp
/// @warning Do not use this type for time point representation!
/// It is very limited and intended for internal MongoDB use.
class Timestamp {
 public:
  /// @brief Creates an empty timestamp
  /// @note MongoDB only replaces empty timestamps in top-level fields.
  Timestamp();

  /// Creates a timestamp with specified values
  Timestamp(uint32_t timestamp, uint32_t increment);

  /// Returns stored unix timestamp
  time_t GetTimestamp() const;

  /// Returns stored increment
  uint32_t GetIncrement() const;

  /// Returns packed 64-bit timestamp value
  uint64_t Pack() const;

  /// Restores a timestamp from the packed form
  static Timestamp Unpack(uint64_t);

  bool operator==(const Timestamp&) const;
  bool operator!=(const Timestamp&) const;
  bool operator<(const Timestamp&) const;
  bool operator>(const Timestamp&) const;
  bool operator<=(const Timestamp&) const;
  bool operator>=(const Timestamp&) const;

 private:
  uint32_t timestamp_{0};
  uint32_t increment_{0};
};

}  // namespace formats::bson

USERVER_NAMESPACE_END

namespace std {

template <>
struct hash<USERVER_NAMESPACE::formats::bson::Oid> {
  size_t operator()(const USERVER_NAMESPACE::formats::bson::Oid&) const;
};

template <>
struct hash<USERVER_NAMESPACE::formats::bson::Binary> {
  size_t operator()(
      const USERVER_NAMESPACE::formats::bson::Binary& binary) const {
    return hash<string>()(binary.data_);
  }
};

template <>
struct hash<USERVER_NAMESPACE::formats::bson::Timestamp> {
  size_t operator()(
      const USERVER_NAMESPACE::formats::bson::Timestamp& timestamp) const;
};

}  // namespace std
