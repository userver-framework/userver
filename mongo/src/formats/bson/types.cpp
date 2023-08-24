#include <userver/formats/bson/types.hpp>

#include <fmt/format.h>

#include <boost/algorithm/string/trim.hpp>

#include <userver/formats/bson/exception.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson {

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Oid::Oid() { bson_oid_init(&oid_, nullptr); }

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Oid::Oid(std::string_view hex_encoded) {
  if (!bson_oid_is_valid(hex_encoded.data(), hex_encoded.size())) {
    throw BsonException(
        fmt::format("Invalid hex-encoded ObjectId: '{}'", hex_encoded));
  }
  bson_oid_init_from_string(&oid_, hex_encoded.data());
}

Oid::Oid(const bson_oid_t& oid) : oid_(oid) {}

Oid Oid::MakeMinimalFor(std::chrono::system_clock::time_point time) {
  bson_oid_t boid{};

  const auto time_as_time_t = std::chrono::system_clock::to_time_t(time);
  auto epoch = static_cast<std::uint32_t>(time_as_time_t);
  if (epoch != time_as_time_t) {
    throw BsonException(
        fmt::format("Can not represent time point {} as std::time_t because "
                    "narrowing happens",
                    utils::datetime::Timestring(time)));
  }

  boid.bytes[3] = epoch & 0xFF;
  epoch = (epoch >> 8);
  boid.bytes[2] = epoch & 0xFF;
  epoch = (epoch >> 8);
  boid.bytes[1] = epoch & 0xFF;
  epoch = (epoch >> 8);
  boid.bytes[0] = epoch & 0xFF;

  return Oid{boid};
}

std::string Oid::ToString() const {
  std::string hex_encoded(Size() * 2 + 1, '\0');
  bson_oid_to_string(&oid_, hex_encoded.data());
  hex_encoded.pop_back();  // drop terminator
  return hex_encoded;
}

const uint8_t* Oid::Data() const { return oid_.bytes; }

time_t Oid::GetTimestamp() const { return bson_oid_get_time_t(&oid_); }

std::chrono::system_clock::time_point Oid::GetTimePoint() const {
  return std::chrono::system_clock::from_time_t(GetTimestamp());
}

const bson_oid_t* Oid::GetNative() const { return &oid_; }

bool Oid::operator==(const Oid& rhs) const {
  return bson_oid_equal(&oid_, &rhs.oid_);
}

bool Oid::operator!=(const Oid& rhs) const { return !(*this == rhs); }

bool Oid::operator<(const Oid& rhs) const {
  return bson_oid_compare(&oid_, &rhs.oid_) < 0;
}

bool Oid::operator>(const Oid& rhs) const {
  return bson_oid_compare(&oid_, &rhs.oid_) > 0;
}

bool Oid::operator<=(const Oid& rhs) const { return !(*this > rhs); }
bool Oid::operator>=(const Oid& rhs) const { return !(*this < rhs); }

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Decimal128::Decimal128(const std::string& value) {
  if (!utils::text::IsAscii(value)) {
    throw BsonException("Non-ASCII chars found during Decimal128 parse");
  }
  if (!bson_decimal128_from_string_w_len(value.data(), value.size(),
                                         &decimal_)) {
    throw BsonException("Invalid Decimal128: '" + value + '\'');
  }
}

Decimal128::Decimal128(const bson_decimal128_t& decimal) : decimal_(decimal) {}

std::string Decimal128::ToString() const {
  std::string result(BSON_DECIMAL128_STRING, '\0');
  bson_decimal128_to_string(&decimal_, result.data());
  boost::algorithm::trim_right_if(result, [](char c) { return !c; });
  return result;
}

Decimal128 Decimal128::Infinity() {
  static const Decimal128 kInfinity(BSON_DECIMAL128_INF);
  return kInfinity;
}

Decimal128 Decimal128::NaN() {
  static const Decimal128 kNaN(BSON_DECIMAL128_NAN);
  return kNaN;
}

const bson_decimal128_t* Decimal128::GetNative() const { return &decimal_; }

bool Decimal128::operator==(const Decimal128& rhs) const {
  return decimal_.high == rhs.decimal_.high && decimal_.low == rhs.decimal_.low;
}

bool Decimal128::operator!=(const Decimal128& rhs) const {
  return !(*this == rhs);
}

Timestamp::Timestamp() = default;

Timestamp::Timestamp(uint32_t timestamp, uint32_t increment)
    : timestamp_(timestamp), increment_(increment) {}

time_t Timestamp::GetTimestamp() const { return timestamp_; }

uint32_t Timestamp::GetIncrement() const { return increment_; }

uint64_t Timestamp::Pack() const {
  return (static_cast<uint64_t>(GetTimestamp()) << 32) | GetIncrement();
}

Timestamp Timestamp::Unpack(uint64_t packed) {
  return {static_cast<uint32_t>(packed >> 32),
          static_cast<uint32_t>(packed & ((std::uint64_t{1} << 32) - 1))};
}

bool Timestamp::operator==(const Timestamp& rhs) const {
  return timestamp_ == rhs.timestamp_ && increment_ == rhs.increment_;
}

bool Timestamp::operator!=(const Timestamp& rhs) const {
  return !(*this == rhs);
}

bool Timestamp::operator<(const Timestamp& rhs) const {
  return timestamp_ < rhs.timestamp_ ||
         (timestamp_ == rhs.timestamp_ && increment_ < rhs.increment_);
}

bool Timestamp::operator>(const Timestamp& rhs) const {
  return !(*this < rhs || *this == rhs);
}

bool Timestamp::operator<=(const Timestamp& rhs) const {
  return !(*this > rhs);
}

bool Timestamp::operator>=(const Timestamp& rhs) const {
  return !(*this < rhs);
}

}  // namespace formats::bson

USERVER_NAMESPACE_END

// NOLINTNEXTLINE(cert-dcl58-cpp): FP, definitions of specializations
namespace std {

size_t hash<USERVER_NAMESPACE::formats::bson::Oid>::operator()(
    const USERVER_NAMESPACE::formats::bson::Oid& oid) const {
  return bson_oid_hash(&oid.oid_);
}

size_t hash<USERVER_NAMESPACE::formats::bson::Timestamp>::operator()(
    const USERVER_NAMESPACE::formats::bson::Timestamp& timestamp) const {
  return hash<uint64_t>{}(timestamp.Pack());
}

}  // namespace std
