#include <userver/formats/json/value.hpp>

#include <cmath>
#include <functional>
#include <limits>

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>

#include <userver/formats/json/exception.hpp>

#include <formats/json/impl/exttypes.hpp>
#include <formats/json/impl/json_tree.hpp>
#include <formats/json/impl/types_impl.hpp>
#include <userver/formats/common/path.hpp>
#include <userver/formats/json/impl/mutable_value_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

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

::rapidjson::CrtAllocator g_allocator;

template <typename T>
auto CheckedNotTooNegative(T x, const Value& value) {
  if (x <= -1) {
    throw ConversionException(
        "Cannot convert to unsigned value from negative " + value.GetPath() +
        '=' + std::to_string(x));
  }
  return x;
}

bool IsIntegral(const double val) {
  double integral_part = NAN;
  return modf(val, &integral_part) == 0.0;
}

constexpr int64_t kMaxIntDouble{int64_t{1}
                                << std::numeric_limits<double>::digits};

template <typename Int>
bool IsNonOverflowingIntegral(const double val) {
  if constexpr (sizeof(Int) >= sizeof(double)) {
    return val > -kMaxIntDouble && val < kMaxIntDouble && IsIntegral(val);
  } else {
    return val >= std::numeric_limits<Int>::min() &&
           val <= std::numeric_limits<Int>::max() && IsIntegral(val);
  }
}
}  // namespace

Value::Value(impl::VersionedValuePtr root) noexcept
    : root_(std::move(root)), value_ptr_(root_.Get()) {}

Value::Value(EmplaceEnabler, const impl::VersionedValuePtr& root,
             const impl::Value& value, int depth)
    : Value(root, &value, depth) {}

Value::Value(impl::VersionedValuePtr root, const impl::Value* value_ptr,
             int depth)
    : root_(std::move(root)),
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
      value_ptr_(const_cast<impl::Value*>(value_ptr)),
      depth_(depth) {}

Value::Value(impl::VersionedValuePtr root, std::string&& detached_path)
    : root_(std::move(root)), detached_path_(detached_path) {}

Value Value::operator[](std::string_view key) const {
  if (!IsMissing()) {
    CheckObjectOrNull();
    if (IsObject()) {
      auto it = GetNative().FindMember(
          impl::Value(::rapidjson::StringRef(key.data(), key.size())));
      if (it != GetNative().MemberEnd()) {
        return {root_, &it->value, depth_ + 1};
      }
    }
  }
  return {root_, formats::common::MakeChildPath(GetPath(), key)};
}

Value Value::operator[](std::size_t index) const {
  CheckInBounds(index);
  return {root_, &GetNative()[static_cast<int>(index)], depth_ + 1};
}

Value::const_iterator Value::begin() const {
  CheckObjectOrArrayOrNull();
  return const_iterator{*this, 0};
}

Value::const_iterator Value::end() const {
  CheckObjectOrArrayOrNull();
  return const_iterator{*this, static_cast<int>(GetSize())};
}

Value::const_reverse_iterator Value::rbegin() const {
  CheckArrayOrNull();
  return const_reverse_iterator{*this, static_cast<int>(GetSize() - 1)};
}

Value::const_reverse_iterator Value::rend() const {
  CheckArrayOrNull();
  return const_reverse_iterator{*this, -1};
}

bool Value::IsEmpty() const {
  CheckObjectOrArrayOrNull();
  if (IsArray()) {
    return GetNative().GetArray().Empty();
  } else if (IsObject()) {
    return GetNative().GetObject().ObjectEmpty();
  } else {
    return true;  // nulls are "empty arrays"
  }
}

std::size_t Value::GetSize() const {
  CheckObjectOrArrayOrNull();
  if (IsArray()) {
    return GetNative().GetArray().Size();
  } else if (IsObject()) {
    return GetNative().GetObject().MemberCount();
  } else {
    return 0;  // nulls are "empty arrays"
  }
}

bool Value::operator==(const Value& other) const {
  return GetNative() == other.GetNative();
}

bool Value::operator!=(const Value& other) const {
  return GetNative() != other.GetNative();
}

bool Value::IsMissing() const noexcept { return root_ && !value_ptr_; }

bool Value::IsNull() const noexcept {
  return !IsMissing() && (!root_ || GetNative().IsNull());
}

bool Value::IsBool() const noexcept {
  return !IsMissing() && GetNative().IsBool();
}

bool Value::IsInt() const noexcept {
  if (IsMissing()) return false;
  const auto& native = GetNative();
  if (native.IsInt()) return true;
  if (native.IsDouble()) {
    return IsNonOverflowingIntegral<int>(native.GetDouble());
  }
  return false;
}

bool Value::IsInt64() const noexcept {
  if (IsMissing()) return false;
  const auto& native = GetNative();
  if (native.IsInt64()) return true;
  if (native.IsDouble()) {
    return IsNonOverflowingIntegral<int64_t>(native.GetDouble());
  }
  return false;
}

bool Value::IsUInt64() const noexcept {
  if (IsMissing()) return false;
  const auto& native = GetNative();
  if (native.IsUint64()) return true;
  if (native.IsDouble()) {
    return IsNonOverflowingIntegral<uint64_t>(native.GetDouble());
  }
  return false;
}

bool Value::IsDouble() const noexcept {
  return !IsMissing() && GetNative().IsNumber();
}

bool Value::IsString() const noexcept {
  return !IsMissing() && GetNative().IsString();
}
bool Value::IsArray() const noexcept {
  return !IsMissing() && GetNative().IsArray();
}
bool Value::IsObject() const noexcept {
  return !IsMissing() && GetNative().IsObject();
}

template <>
bool Value::As<bool>() const {
  CheckNotMissing();
  const auto& native = GetNative();
  if (native.IsTrue()) return true;
  if (native.IsFalse()) return false;
  throw TypeMismatchException(GetExtendedType(), impl::booleanValue, GetPath());
}

template <>
int64_t Value::As<int64_t>() const {
  CheckNotMissing();
  const auto& native = GetNative();
  if (native.IsInt64()) return native.GetInt64();
  if (native.IsDouble()) {
    double val = native.GetDouble();
    if (IsNonOverflowingIntegral<int64_t>(val))
      return static_cast<int64_t>(val);
  }
  throw TypeMismatchException(GetExtendedType(), impl::intValue, GetPath());
}

template <>
uint64_t Value::As<uint64_t>() const {
  CheckNotMissing();
  const auto& native = GetNative();
  if (native.IsUint64()) return native.GetUint64();
  if (native.IsDouble()) {
    double val = native.GetDouble();
    if (IsNonOverflowingIntegral<uint64_t>(val))
      return static_cast<uint64_t>(val);
  }
  throw TypeMismatchException(GetExtendedType(), impl::uintValue, GetPath());
}

template <>
double Value::As<double>() const {
  CheckNotMissing();
  const auto& native = GetNative();
  if (native.IsDouble()) return native.GetDouble();
  if (native.IsInt64()) return static_cast<double>(native.GetInt64());
  if (native.IsUint64()) return static_cast<double>(native.GetUint64());
  throw TypeMismatchException(GetExtendedType(), impl::realValue, GetPath());
}

template <>
std::string Value::As<std::string>() const {
  CheckNotMissing();
  const auto& native = GetNative();
  if (native.IsString()) return {native.GetString(), native.GetStringLength()};
  throw TypeMismatchException(GetExtendedType(), impl::stringValue, GetPath());
}

template <>
bool Value::ConvertTo<bool>() const {
  if (IsMissing()) return false;
  const auto& native = GetNative();
  if (native.IsNull() || native.IsFalse()) return false;
  if (native.IsTrue()) return true;
  if (native.IsInt64()) return GetNative().GetInt64() != 0;
  if (native.IsUint64()) return GetNative().GetUint64() != 0;
  if (native.IsDouble())
    return std::fabs(native.GetDouble()) >
           std::numeric_limits<double>::epsilon();
  if (native.IsString()) return GetNative().GetStringLength() != 0;
  if (native.IsArray()) return native.Size() != 0;
  if (native.IsObject()) return native.MemberCount() != 0;

  throw TypeMismatchException(GetExtendedType(), impl::booleanValue, GetPath());
}

template <>
int64_t Value::ConvertTo<int64_t>() const {
  if (IsMissing()) return false;
  const auto& native = GetNative();
  if (native.IsNull() || native.IsFalse()) return 0;
  if (native.IsTrue()) return 1;
  if (native.IsInt64()) return native.GetInt64();
  if (native.IsUint64()) return native.GetUint64();
  if (native.IsDouble()) return static_cast<int64_t>(native.GetDouble());

  throw TypeMismatchException(GetExtendedType(), impl::intValue, GetPath());
}

template <>
uint64_t Value::ConvertTo<uint64_t>() const {
  if (IsMissing()) return false;
  const auto& native = GetNative();
  if (native.IsNull() || native.IsFalse()) return 0;
  if (native.IsTrue()) return 1;
  if (native.IsInt64())
    return static_cast<uint64_t>(
        CheckedNotTooNegative(native.GetInt64(), *this));
  if (native.IsUint64()) return native.GetUint64();
  if (native.IsDouble())
    return static_cast<uint64_t>(
        CheckedNotTooNegative(native.GetDouble(), *this));

  throw TypeMismatchException(GetExtendedType(), impl::uintValue, GetPath());
}

template <>
double Value::ConvertTo<double>() const {
  if (!IsMissing() && IsDouble()) return As<double>();
  return ConvertTo<int64_t>();
}

template <>
std::string Value::ConvertTo<std::string>() const {
  if (IsMissing()) return {};
  const auto& native = GetNative();
  if (native.IsNull()) return {};
  if (native.IsString()) return As<std::string>();

  if (native.IsTrue()) return "true";
  if (native.IsFalse()) return "false";

  if (native.IsInt64()) return std::to_string(native.GetInt64());
  if (native.IsUint64()) return std::to_string(native.GetUint64());
  if (native.IsDouble()) return std::to_string(native.GetDouble());

  throw TypeMismatchException(GetExtendedType(), impl::stringValue, GetPath());
}

bool Value::HasMember(std::string_view key) const {
  if (IsMissing()) return false;
  CheckObjectOrNull();
  return IsObject() && GetNative().HasMember(impl::Value(
                           ::rapidjson::StringRef(key.data(), key.size())));
}

std::string Value::GetPath() const {
  if (value_ptr_ != nullptr) {
    return impl::MakePath(root_.Get(), value_ptr_, depth_);
  } else {
    return detached_path_.empty() ? formats::common::kPathRoot : detached_path_;
  }
}

Value Value::Clone() const {
  return Value{impl::VersionedValuePtr::Create(GetNative(), g_allocator)};
}

// Value states
//
// !!root_ | !!value_ptr_ | Description
// ------- | ------------ | ------------------
//  false  |     false    | Implicitly `kNull`
//  false  |     true     | ---
//  true   |     false    | Missing
//  true   |     true     | Valid
void Value::EnsureNotMissing() {
  CheckNotMissing();
  if (!root_) {
    root_ = impl::VersionedValuePtr::Create(::rapidjson::Type::kNullType);
    value_ptr_ = root_.Get();
  }
}

bool Value::IsRoot() const noexcept { return root_.Get() == value_ptr_; }

bool Value::IsUniqueReference() const { return root_.IsUnique(); }

const impl::Value& Value::GetNative() const {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  const_cast<Value*>(this)->EnsureNotMissing();
  return *value_ptr_;
}

impl::Value& Value::GetNative() {
  EnsureNotMissing();
  return *value_ptr_;
}

void Value::SetNative(impl::Value& value) {
  CheckNotMissing();
  value_ptr_ = &value;
}

// convert internal rapidjson type to implementation-specific type that
// distinguishes between int/uint/double
int Value::GetExtendedType() const {
  return impl::GetExtendedType(GetNative());
}

void Value::CheckNotMissing() const {
  if (IsMissing()) {
    throw MemberMissingException(GetPath());
  }
}

void Value::CheckArrayOrNull() const {
  if (!IsNull() && !IsArray()) {
    throw TypeMismatchException(GetExtendedType(), impl::arrayValue, GetPath());
  }
}

void Value::CheckObjectOrNull() const {
  if (!IsNull() && !IsObject()) {
    throw TypeMismatchException(GetExtendedType(), impl::objectValue,
                                GetPath());
  }
}

void Value::CheckObject() const {
  if (!IsObject()) {
    throw TypeMismatchException(GetExtendedType(), impl::objectValue,
                                GetPath());
  }
}

void Value::CheckObjectOrArrayOrNull() const {
  if (!IsNull() && !IsObject() && !IsArray()) {
    throw TypeMismatchException(GetExtendedType(), impl::objectValue,
                                GetPath());
  }
}

void Value::CheckInBounds(std::size_t index) const {
  CheckArrayOrNull();
  if (index >= GetSize()) {
    throw OutOfBoundsException(index, GetSize(), GetPath());
  }
}
}  // namespace formats::json

namespace formats::literals {

json::Value operator"" _json(const char* str, size_t len) {
  return json::FromString(std::string_view(str, len));
}

}  // namespace formats::literals

USERVER_NAMESPACE_END
