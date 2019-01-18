#include <formats/bson/types.hpp>

#include <boost/algorithm/string/trim.hpp>

#include <formats/bson/exception.hpp>
#include <utils/text.hpp>

namespace formats::bson {

Oid::Oid(const std::string& hex_encoded) {
  if (!bson_oid_is_valid(hex_encoded.data(), hex_encoded.size())) {
    throw BsonException("Invalid hex-encoded ObjectId: '" + hex_encoded + '\'');
  }
  bson_oid_init_from_string(&oid_, hex_encoded.c_str());
}

Oid::Oid(const bson_oid_t& oid) : oid_(oid) {}

std::string Oid::ToString() const {
  std::string hex_encoded(Size() * 2 + 1, '\0');
  bson_oid_to_string(&oid_, &hex_encoded[0]);
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
  bson_decimal128_to_string(&decimal_, &result[0]);
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

}  // namespace formats::bson

namespace std {

size_t hash<formats::bson::Oid>::operator()(
    const formats::bson::Oid& oid) const {
  return bson_oid_hash(&oid.oid_);
}

}  // namespace std
