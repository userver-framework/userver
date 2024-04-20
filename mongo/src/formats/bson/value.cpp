#include <userver/formats/bson/value.hpp>

#include <cmath>
#include <limits>
#include <string>

#include <formats/bson/value_impl.hpp>
#include <formats/bson/wrappers.hpp>
#include <userver/formats/bson/bson_builder.hpp>
#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/serialize.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson {
namespace {

static_assert(std::numeric_limits<double>::radix == 2,
              "Your compiler provides double with an unusual radix, please "
              "contact userver support chat");
static_assert(std::numeric_limits<double>::digits >=
                  std::numeric_limits<int32_t>::digits,
              "Your compiler provides unusually small double, please contact "
              "userver support chat");
static_assert(std::numeric_limits<double>::digits <
                  std::numeric_limits<int64_t>::digits,
              "Your compiler provides unusually large double, please contact "
              "userver support chat");

constexpr std::int64_t kMaxIntDouble{std::int64_t{1}
                                     << std::numeric_limits<double>::digits};

template <typename T>
auto CheckedNotTooNegative(T x, const Value& value) {
  if (x <= -1) {
    throw ConversionException(
        utils::StrCat("Cannot convert to unsigned value from negative value ",
                      std::to_string(x)),
        value.GetPath());
  }
  return x;
}

Document AsDocumentUnchecked(const impl::ValueImpl& value_impl) {
  if (value_impl.IsStorageOwner()) {
    return Document(value_impl.GetBson());
  } else {
    const auto& bson_value = value_impl.GetNative();
    return Document(impl::MutableBson(bson_value->value.v_doc.data,
                                      bson_value->value.v_doc.data_len)
                        .Extract());
  }
}

}  // namespace

Value::Value() : impl_(std::make_shared<impl::ValueImpl>(nullptr)) {}

Value::Value(impl::ValueImplPtr impl) : impl_(std::move(impl)) {}

Value Value::operator[](const std::string& name) const {
  return Value((*impl_)[name]);
}

Value Value::operator[](uint32_t index) const { return Value((*impl_)[index]); }

bool Value::HasMember(const std::string& name) const {
  return impl_->HasMember(name);
}

Value::const_iterator Value::begin() const { return {*impl_, impl_->Begin()}; }
Value::const_iterator Value::end() const { return {*impl_, impl_->End()}; }

Value::const_reverse_iterator Value::rbegin() const {
  CheckArrayOrNull();
  return {*impl_, impl_->Rbegin()};
}

Value::const_reverse_iterator Value::rend() const {
  CheckArrayOrNull();
  return {*impl_, impl_->Rend()};
}

bool Value::IsEmpty() const { return impl_->IsEmpty(); }
uint32_t Value::GetSize() const { return impl_->GetSize(); }
std::string Value::GetPath() const { return impl_->GetPath(); }

bool Value::operator==(const Value& rhs) const { return *impl_ == *rhs.impl_; }
bool Value::operator!=(const Value& rhs) const { return *impl_ != *rhs.impl_; }

bool Value::IsMissing() const { return impl_->IsMissing(); }
bool Value::IsArray() const { return impl_->IsArray(); }
bool Value::IsDocument() const { return impl_->IsDocument(); }
bool Value::IsNull() const { return impl_->IsNull(); }
bool Value::IsBool() const { return impl_->Type() == BSON_TYPE_BOOL; }
bool Value::IsInt32() const { return impl_->Type() == BSON_TYPE_INT32; }

bool Value::IsInt64() const {
  return impl_->Type() == BSON_TYPE_INT64 || IsInt32();
}

bool Value::IsDouble() const {
  return impl_->Type() == BSON_TYPE_DOUBLE || IsInt64();
}

bool Value::IsString() const { return impl_->Type() == BSON_TYPE_UTF8; }
bool Value::IsDateTime() const { return impl_->Type() == BSON_TYPE_DATE_TIME; }
bool Value::IsOid() const { return impl_->Type() == BSON_TYPE_OID; }
bool Value::IsBinary() const { return impl_->Type() == BSON_TYPE_BINARY; }

bool Value::IsDecimal128() const {
  return impl_->Type() == BSON_TYPE_DECIMAL128;
}

bool Value::IsMinKey() const { return impl_->Type() == BSON_TYPE_MINKEY; }
bool Value::IsMaxKey() const { return impl_->Type() == BSON_TYPE_MAXKEY; }
bool Value::IsTimestamp() const { return impl_->Type() == BSON_TYPE_TIMESTAMP; }

bool Parse(const Value& value, parse::To<bool>) {
  value.CheckNotMissing();
  if (value.IsBool()) return value.impl_->GetNative()->value.v_bool;
  throw TypeMismatchException(value.impl_->Type(), BSON_TYPE_BOOL,
                              value.GetPath());
}

int64_t Parse(const Value& value, parse::To<int64_t>) {
  value.CheckNotMissing();
  if (value.IsInt32()) return value.impl_->GetNative()->value.v_int32;
  if (value.IsInt64()) return value.impl_->GetNative()->value.v_int64;
  if (value.IsDouble()) {
    auto as_double = value.As<double>();
    double int_part = 0.0;
    auto frac_part = std::modf(as_double, &int_part);
    if (frac_part || std::abs(as_double) >= kMaxIntDouble) {
      throw ConversionException(
          utils::StrCat("Conversion ", std::to_string(as_double),
                        " to integer causes precision change"),
          value.GetPath());
    }
    return static_cast<int64_t>(as_double);
  }
  throw TypeMismatchException(value.impl_->Type(), BSON_TYPE_INT64,
                              value.GetPath());
}

uint64_t Parse(const Value& value, parse::To<uint64_t>) {
  value.CheckNotMissing();
  if (value.IsInt64() || value.IsInt32() || value.IsDouble()) {
    return static_cast<uint64_t>(
        CheckedNotTooNegative(value.As<int64_t>(), value));
  }
  throw TypeMismatchException(value.impl_->Type(), BSON_TYPE_INT64,
                              value.GetPath());
}

double Parse(const Value& value, parse::To<double>) {
  value.CheckNotMissing();
  if (value.IsInt32()) return static_cast<double>(value.As<int64_t>());
  if (value.IsInt64()) {
    auto as_int = value.As<int64_t>();
    if (as_int == std::numeric_limits<int64_t>::min() ||
        std::abs(as_int) > kMaxIntDouble) {
      throw ConversionException(
          utils::StrCat("Conversion of ", std::to_string(as_int),
                        " to double causes precision loss"),
          value.GetPath());
    }
    return static_cast<double>(as_int);
  }
  if (value.IsDouble()) return value.impl_->GetNative()->value.v_double;
  throw TypeMismatchException(value.impl_->Type(), BSON_TYPE_DOUBLE,
                              value.GetPath());
}

std::string Parse(const Value& value, parse::To<std::string>) {
  value.CheckNotMissing();
  if (value.IsString()) {
    const auto& str = value.impl_->GetNative()->value.v_utf8;
    return {str.str, str.len};
  }
  throw TypeMismatchException(value.impl_->Type(), BSON_TYPE_UTF8,
                              value.GetPath());
}

std::chrono::system_clock::time_point Parse(
    const Value& value, parse::To<std::chrono::system_clock::time_point>) {
  value.CheckNotMissing();
  if (value.IsDateTime())
    return std::chrono::system_clock::time_point(
        std::chrono::milliseconds(value.impl_->GetNative()->value.v_datetime));
  throw TypeMismatchException(value.impl_->Type(), BSON_TYPE_DATE_TIME,
                              value.GetPath());
}

Oid Parse(const Value& value, parse::To<Oid>) {
  value.CheckNotMissing();
  if (value.IsOid()) return value.impl_->GetNative()->value.v_oid;
  throw TypeMismatchException(value.impl_->Type(), BSON_TYPE_OID,
                              value.GetPath());
}

Binary Parse(const Value& value, parse::To<Binary>) {
  value.CheckNotMissing();
  if (value.IsBinary()) {
    const auto& data = value.impl_->GetNative()->value.v_binary;
    return Binary(
        std::string(reinterpret_cast<const char*>(data.data), data.data_len));
  }
  throw TypeMismatchException(value.impl_->Type(), BSON_TYPE_BINARY,
                              value.GetPath());
}

Decimal128 Parse(const Value& value, parse::To<Decimal128>) {
  value.CheckNotMissing();
  if (value.IsDecimal128()) return value.impl_->GetNative()->value.v_decimal128;
  throw TypeMismatchException(value.impl_->Type(), BSON_TYPE_DECIMAL128,
                              value.GetPath());
}

Timestamp Parse(const Value& value, parse::To<Timestamp>) {
  value.CheckNotMissing();
  if (value.IsTimestamp()) {
    return {value.impl_->GetNative()->value.v_timestamp.timestamp,
            value.impl_->GetNative()->value.v_timestamp.increment};
  }
  throw TypeMismatchException(value.impl_->Type(), BSON_TYPE_TIMESTAMP,
                              value.GetPath());
}

Document Parse(const Value& value, parse::To<Document>) {
  value.CheckNotMissing();
  value.impl_->CheckIsDocument();
  return AsDocumentUnchecked(*value.impl_);
}

template <>
bool Value::ConvertTo<bool>() const {
  if (IsMissing() || IsNull()) return false;
  if (IsBool()) return As<bool>();
  if (IsInt64() || IsInt32()) return As<int64_t>();
  if (IsDouble())
    return fabs(As<double>()) > std::numeric_limits<double>::epsilon();
  if (IsDateTime()) return true;
  if (IsString()) return impl_->GetNative()->value.v_utf8.len;
  if (IsBinary()) return impl_->GetNative()->value.v_binary.data_len;
  if (IsArray() || IsDocument()) return GetSize();

  throw TypeMismatchException(impl_->Type(), BSON_TYPE_BOOL, GetPath());
}

template <>
int64_t Value::ConvertTo<int64_t>() const {
  if (IsMissing() || IsNull()) return 0;
  if (IsBool()) return As<bool>();
  if (IsInt64() || IsInt32()) return As<int64_t>();
  if (IsDouble()) return static_cast<int64_t>(As<double>());
  if (IsDateTime()) return impl_->GetNative()->value.v_datetime;

  throw TypeMismatchException(impl_->Type(), BSON_TYPE_INT64, GetPath());
}

template <>
uint64_t Value::ConvertTo<uint64_t>() const {
  if (IsDouble() && !(IsInt64() || IsInt32())) {
    return static_cast<uint64_t>(CheckedNotTooNegative(As<double>(), *this));
  }
  return static_cast<uint64_t>(
      CheckedNotTooNegative(ConvertTo<int64_t>(), *this));
}

template <>
double Value::ConvertTo<double>() const {
  if (IsDouble()) return As<double>();
  return ConvertTo<int64_t>();
}

template <>
std::string Value::ConvertTo<std::string>() const {
  if (IsString()) return As<std::string>();
  if (IsBinary()) return As<Binary>().ToString();

  if (IsMissing() || IsNull()) return {};
  if (IsBool()) return As<bool>() ? "true" : "false";
  if (IsInt64() || IsDateTime()) return std::to_string(ConvertTo<int64_t>());
  if (IsDouble()) return std::to_string(As<double>());
  if (IsOid()) return As<Oid>().ToString();

  throw TypeMismatchException(impl_->Type(), BSON_TYPE_UTF8, GetPath());
}

void Value::SetDuplicateFieldsPolicy(DuplicateFieldsPolicy policy) {
  impl_->SetDuplicateFieldsPolicy(policy);
}

void Value::CheckNotMissing() const { impl_->CheckNotMissing(); }

void Value::CheckArrayOrNull() const {
  if (IsNull()) return;
  impl_->CheckIsArray();
}

void Value::CheckDocumentOrNull() const {
  if (IsNull()) return;
  impl_->CheckIsDocument();
}

Document Value::GetInternalArrayDocument() const {
  CheckNotMissing();
  impl_->CheckIsArray();
  return AsDocumentUnchecked(*impl_);
}

const impl::BsonHolder& Value::GetBson() const { return impl_->GetBson(); }

}  // namespace formats::bson

namespace formats::literals {

bson::Value operator"" _bson(const char* str, size_t len) {
  return bson::FromJsonString(std::string(str, len));
}

}  // namespace formats::literals

USERVER_NAMESPACE_END
