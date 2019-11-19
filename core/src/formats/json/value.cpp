#include <formats/json/value.hpp>

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <formats/json/exception.hpp>
#include <functional>

#include "exttypes.hpp"

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

rapidjson::CrtAllocator g_crt_allocator;

template <typename T>
auto CheckedNotTooNegative(T x, const Value& value) {
  if (x <= -1) {
    throw ConversionException(
        "Cannot convert to unsigned value from negative " + value.GetPath() +
        '=' + std::to_string(x));
  }
  return x;
};

bool IsIntegral(const double val) {
  double integral_part;
  return modf(val, &integral_part) == 0.0;
}

template <typename Int>
bool IsNonOverflowingIntegral(const double val) {
  if constexpr (sizeof(Int) >= sizeof(double)) {
    // to avoid overflow problems of integers wider that double's mantissa we
    // require "strict less than" check on upper boundary
    return val >= std::numeric_limits<Int>::min() &&
           val < std::numeric_limits<Int>::max() && IsIntegral(val);
  } else {
    return val >= std::numeric_limits<Int>::min() &&
           val <= std::numeric_limits<Int>::max() && IsIntegral(val);
  }
}
}  // namespace

Value::Value() noexcept : value_ptr_(nullptr) {}

Value::~Value() = default;

Value::Value(NativeValuePtr&& root) noexcept
    : root_(std::move(root)), value_ptr_(root_.get()) {}

Value::Value(const NativeValuePtr& root, const impl::Value* value_ptr,
             const formats::json::Path& path, const std::string& key)
    : root_(root),
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
      value_ptr_(const_cast<impl::Value*>(value_ptr)),
      path_(path.MakeChildPath(key)) {}

Value::Value(const NativeValuePtr& root, const impl::Value& val,
             const formats::json::Path& path, std::size_t index)
    : root_(root),
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
      value_ptr_(const_cast<impl::Value*>(&val)),
      path_(path.MakeChildPath(index)) {}

Value Value::operator[](const std::string& key) const {
  const impl::Value* child = nullptr;
  if (!IsMissing()) {
    CheckObjectOrNull();
    if (IsObject()) {
      GetNative().FindMember(key);
      auto object = GetNative().GetObject();
      auto it = object.FindMember(key);
      if (it != object.end()) {
        child = &it->value;
      }
    }
  }
  return {root_, child, path_, key};
}

Value Value::operator[](std::size_t index) const {
  CheckInBounds(index);
  return {root_, GetNative()[static_cast<int>(index)], path_, index};
}

Value::const_iterator Value::begin() const {
  CheckObjectOrArrayOrNull();
  return {root_, &GetNative(), 0, path_};
}

Value::const_iterator Value::end() const {
  CheckObjectOrArrayOrNull();
  return {root_, &GetNative(), static_cast<int>(GetSize()), path_};
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

bool Value::IsNull() const { return !IsMissing() && GetNative().IsNull(); }
bool Value::IsBool() const { return !IsMissing() && GetNative().IsBool(); }

bool Value::IsInt() const {
  if (IsMissing()) return false;
  const auto& native = GetNative();
  if (native.IsInt()) return true;
  if (native.IsDouble()) {
    return IsNonOverflowingIntegral<int>(native.GetDouble());
  }
  return false;
}

bool Value::IsInt64() const {
  if (IsMissing()) return false;
  const auto& native = GetNative();
  if (native.IsInt64()) return true;
  if (native.IsDouble()) {
    return IsNonOverflowingIntegral<int64_t>(native.GetDouble());
  }
  return false;
}

bool Value::IsUInt64() const {
  if (IsMissing()) return false;
  const auto& native = GetNative();
  if (native.IsUint64()) return true;
  if (native.IsDouble()) {
    return IsNonOverflowingIntegral<uint64_t>(native.GetDouble());
  }
  return false;
}

bool Value::IsDouble() const { return !IsMissing() && GetNative().IsNumber(); }

bool Value::IsString() const { return !IsMissing() && GetNative().IsString(); }
bool Value::IsArray() const { return !IsMissing() && GetNative().IsArray(); }
bool Value::IsObject() const { return !IsMissing() && GetNative().IsObject(); }

template <>
bool Value::As<bool>() const {
  CheckNotMissing();
  const auto& native = GetNative();
  if (native.IsTrue()) return true;
  if (native.IsFalse()) return false;
  throw TypeMismatchException(GetExtendedType(), impl::booleanValue,
                              path_.ToString());
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
  throw TypeMismatchException(GetExtendedType(), impl::intValue,
                              path_.ToString());
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
  throw TypeMismatchException(GetExtendedType(), impl::uintValue,
                              path_.ToString());
}

template <>
double Value::As<double>() const {
  CheckNotMissing();
  const auto& native = GetNative();
  if (native.IsDouble()) return native.GetDouble();
  if (native.IsInt64()) return static_cast<double>(native.GetInt64());
  if (native.IsUint64()) return static_cast<double>(native.GetUint64());
  throw TypeMismatchException(GetExtendedType(), impl::realValue,
                              path_.ToString());
}

template <>
std::string Value::As<std::string>() const {
  CheckNotMissing();
  const auto& native = GetNative();
  if (native.IsString()) return native.GetString();
  throw TypeMismatchException(GetExtendedType(), impl::stringValue,
                              path_.ToString());
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

  throw TypeMismatchException(GetExtendedType(), impl::booleanValue,
                              path_.ToString());
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

  throw TypeMismatchException(GetExtendedType(), impl::intValue,
                              path_.ToString());
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

  throw TypeMismatchException(GetExtendedType(), impl::uintValue,
                              path_.ToString());
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

  throw TypeMismatchException(GetExtendedType(), impl::stringValue,
                              path_.ToString());
}

bool Value::HasMember(const char* key) const {
  if (IsMissing()) return false;
  CheckObjectOrNull();
  return IsObject() && GetNative().HasMember(key);
}

bool Value::HasMember(const std::string& key) const {
  if (IsMissing()) return false;
  CheckObjectOrNull();
  return IsObject() && GetNative().HasMember(key);
}

std::string Value::GetPath() const { return path_.ToString(); }

Value Value::Clone() const {
  auto result{std::make_shared<impl::Value>()};
  result->CopyFrom(GetNative(), g_crt_allocator);
  return Value{std::move(result)};
}

void Value::SetNonRoot(const NativeValuePtr& root, const impl::Value& val,
                       const formats::json::Path& path,
                       const std::string& key) {
  *this = Value(root, &val, path, key);
}

void Value::SetNonRoot(const NativeValuePtr& root, const impl::Value& val,
                       const formats::json::Path& path, std::size_t index) {
  *this = Value(root, val, path, index);
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
    root_ = std::make_shared<impl::Value>();
    value_ptr_ = root_.get();
  }
}

bool Value::IsRoot() const noexcept { return root_.get() == value_ptr_; }

bool Value::IsUniqueReference() const { return root_.use_count() == 1; }

const impl::Value& Value::GetNative() const {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  const_cast<Value*>(this)->EnsureNotMissing();
  return *value_ptr_;
}

impl::Value& Value::GetNative() {
  EnsureNotMissing();
  return *value_ptr_;
}

// convert internal rapidjson type to implementation-specific type that
// distinguishes between int/uint/double
int Value::GetExtendedType() const {
  return impl::GetExtendedType(GetNative());
}

void Value::CheckNotMissing() const {
  if (IsMissing()) {
    throw MemberMissingException(path_.ToString());
  }
}

void Value::CheckArrayOrNull() const {
  if (!IsArray() && !IsNull()) {
    throw TypeMismatchException(GetExtendedType(), impl::arrayValue,
                                path_.ToString());
  }
}

void Value::CheckObjectOrNull() const {
  if (!IsObject() && !IsNull()) {
    throw TypeMismatchException(GetExtendedType(), impl::objectValue,
                                path_.ToString());
  }
}

void Value::CheckObject() const {
  if (!IsObject()) {
    throw TypeMismatchException(GetExtendedType(), impl::objectValue,
                                path_.ToString());
  }
}

void Value::CheckObjectOrArrayOrNull() const {
  if (!IsObject() && !IsArray() && !IsNull()) {
    throw TypeMismatchException(GetExtendedType(), impl::objectValue,
                                path_.ToString());
  }
}

void Value::CheckInBounds(std::size_t index) const {
  CheckArrayOrNull();
  if (index >= GetSize()) {
    throw OutOfBoundsException(index, GetSize(), path_.ToString());
  }
}
}  // namespace formats::json
