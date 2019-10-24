#include <formats/json/value.hpp>

#include <formats/json/exception.hpp>

#include <json/value.h>

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

template <typename T>
auto CheckedNotTooNegative(T x, const Value& value) {
  if (x <= -1) {
    throw ConversionException(
        "Cannot convert to unsigned value from negative " + value.GetPath() +
        '=' + std::to_string(x));
  }
  return x;
};

}  // namespace

Value::Value() noexcept : value_ptr_(nullptr) {}

Value::~Value() = default;

Value::Value(NativeValuePtr&& root) noexcept
    : root_(std::move(root)), value_ptr_(root_.get()) {}

Value::Value(const NativeValuePtr& root, const Json::Value* value_ptr,
             const formats::json::Path& path, const std::string& key)
    : root_(root),
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
      value_ptr_(const_cast<Json::Value*>(value_ptr)),
      path_(path.MakeChildPath(key)) {}

Value::Value(const NativeValuePtr& root, const Json::Value& val,
             const formats::json::Path& path, std::size_t index)
    : root_(root),
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
      value_ptr_(const_cast<Json::Value*>(&val)),
      path_(path.MakeChildPath(index)) {}

Value Value::operator[](const std::string& key) const {
  const Json::Value* child = nullptr;
  if (!IsMissing()) {
    CheckObjectOrNull();
    child = GetNative().find(key.data(), key.data() + key.size());
  }
  return {root_, child, path_, key};
}

Value Value::operator[](std::size_t index) const {
  CheckInBounds(index);
  return {root_, GetNative()[static_cast<int>(index)], path_, index};
}

Value::const_iterator Value::begin() const {
  CheckObjectOrArrayOrNull();
  return {root_, GetNative().begin(), path_};
}

Value::const_iterator Value::end() const {
  CheckObjectOrArrayOrNull();
  return {root_, GetNative().end(), path_};
}

std::size_t Value::GetSize() const {
  CheckObjectOrArrayOrNull();
  return GetNative().size();
}

bool Value::operator==(const Value& other) const {
  return GetNative() == other.GetNative();
}

bool Value::operator!=(const Value& other) const {
  return GetNative() != other.GetNative();
}

bool Value::IsMissing() const noexcept { return root_ && !value_ptr_; }

bool Value::IsNull() const { return !IsMissing() && GetNative().isNull(); }
bool Value::IsBool() const { return !IsMissing() && GetNative().isBool(); }
bool Value::IsInt() const { return !IsMissing() && GetNative().isInt(); }
bool Value::IsInt64() const { return !IsMissing() && GetNative().isInt64(); }
bool Value::IsUInt64() const { return !IsMissing() && GetNative().isUInt64(); }
bool Value::IsDouble() const { return !IsMissing() && GetNative().isDouble(); }
bool Value::IsString() const { return !IsMissing() && GetNative().isString(); }
bool Value::IsArray() const { return !IsMissing() && GetNative().isArray(); }
bool Value::IsObject() const { return !IsMissing() && GetNative().isObject(); }

template <>
bool Value::As<bool>() const {
  CheckNotMissing();
  if (IsBool()) return GetNative().asBool();
  throw TypeMismatchException(GetNative().type(), Json::booleanValue,
                              path_.ToString());
}

template <>
int64_t Value::As<int64_t>() const {
  CheckNotMissing();
  if (IsInt64()) return GetNative().asInt64();
  throw TypeMismatchException(GetNative().type(), Json::intValue,
                              path_.ToString());
}

template <>
uint64_t Value::As<uint64_t>() const {
  CheckNotMissing();
  if (IsUInt64()) return GetNative().asUInt64();
  throw TypeMismatchException(GetNative().type(), Json::uintValue,
                              path_.ToString());
}

template <>
double Value::As<double>() const {
  CheckNotMissing();
  if (IsDouble()) return GetNative().asDouble();
  throw TypeMismatchException(GetNative().type(), Json::realValue,
                              path_.ToString());
}

template <>
std::string Value::As<std::string>() const {
  CheckNotMissing();
  if (IsString()) return GetNative().asString();
  throw TypeMismatchException(GetNative().type(), Json::stringValue,
                              path_.ToString());
}

template <>
bool Value::ConvertTo<bool>() const {
  if (IsMissing() || IsNull()) return false;
  if (IsBool()) return GetNative().asBool();
  if (IsInt64()) return GetNative().asInt64();
  if (IsUInt64()) return GetNative().asUInt64();
  if (IsDouble())
    return std::fabs(GetNative().asDouble()) >
           std::numeric_limits<double>::epsilon();
  if (IsString()) return !GetNative().asString().empty();
  if (IsArray() || IsObject()) return GetSize();

  throw TypeMismatchException(GetNative().type(), Json::booleanValue,
                              path_.ToString());
}

template <>
int64_t Value::ConvertTo<int64_t>() const {
  if (IsMissing() || IsNull()) return 0;
  if (IsBool()) return GetNative().asBool();
  if (IsInt64()) return GetNative().asInt64();
  if (IsUInt64()) return GetNative().asUInt64();
  if (IsDouble()) return static_cast<int64_t>(GetNative().asDouble());

  throw TypeMismatchException(GetNative().type(), Json::intValue,
                              path_.ToString());
}

template <>
uint64_t Value::ConvertTo<uint64_t>() const {
  if (IsMissing() || IsNull()) return 0;
  if (IsBool()) return GetNative().asBool();
  if (IsInt64())
    return static_cast<uint64_t>(
        CheckedNotTooNegative(GetNative().asInt64(), *this));
  if (IsUInt64()) return GetNative().asUInt64();
  if (IsDouble())
    return static_cast<uint64_t>(
        CheckedNotTooNegative(GetNative().asDouble(), *this));

  throw TypeMismatchException(GetNative().type(), Json::uintValue,
                              path_.ToString());
}

template <>
double Value::ConvertTo<double>() const {
  if (IsDouble()) return As<double>();
  return ConvertTo<int64_t>();
}

template <>
std::string Value::ConvertTo<std::string>() const {
  if (IsMissing() || IsNull()) return {};
  if (IsString()) return As<std::string>();

  if (IsBool()) return (GetNative().asBool() ? "true" : "false");

  // Json::realValue == double only!
  if (GetNative().type() == Json::realValue)
    return std::to_string(GetNative().asDouble());

  if (IsInt64()) return std::to_string(GetNative().asInt64());
  if (IsUInt64()) return std::to_string(GetNative().asUInt64());

  throw TypeMismatchException(GetNative().type(), Json::stringValue,
                              path_.ToString());
}

bool Value::HasMember(const char* key) const {
  if (IsMissing()) return false;
  CheckObjectOrNull();
  return GetNative().isMember(key);
}

bool Value::HasMember(const std::string& key) const {
  if (IsMissing()) return false;
  CheckObjectOrNull();
  return GetNative().isMember(key);
}

std::string Value::GetPath() const { return path_.ToString(); }

Value Value::Clone() const {
  Value v;
  v.GetNative() = GetNative();
  return v;
}

void Value::SetNonRoot(const NativeValuePtr& root, const Json::Value& val,
                       const formats::json::Path& path,
                       const std::string& key) {
  *this = Value(root, &val, path, key);
}

void Value::SetNonRoot(const NativeValuePtr& root, const Json::Value& val,
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
    root_ = std::make_shared<Json::Value>();
    value_ptr_ = root_.get();
  }
}

bool Value::IsRoot() const noexcept { return root_.get() == value_ptr_; }

bool Value::IsUniqueReference() const { return root_.use_count() == 1; }

const Json::Value& Value::GetNative() const {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  const_cast<Value*>(this)->EnsureNotMissing();
  return *value_ptr_;
}

Json::Value& Value::GetNative() {
  EnsureNotMissing();
  return *value_ptr_;
}

void Value::CheckNotMissing() const {
  if (IsMissing()) {
    throw MemberMissingException(path_.ToString());
  }
}

void Value::CheckArrayOrNull() const {
  if (!IsArray() && !IsNull()) {
    throw TypeMismatchException(GetNative().type(), Json::arrayValue,
                                path_.ToString());
  }
}

void Value::CheckObjectOrNull() const {
  if (!IsObject() && !IsNull()) {
    throw TypeMismatchException(GetNative().type(), Json::objectValue,
                                path_.ToString());
  }
}

void Value::CheckObject() const {
  if (!IsObject()) {
    throw TypeMismatchException(GetNative().type(), Json::objectValue,
                                path_.ToString());
  }
}

void Value::CheckObjectOrArrayOrNull() const {
  if (!IsObject() && !IsArray() && !IsNull()) {
    throw TypeMismatchException(GetNative().type(), Json::objectValue,
                                path_.ToString());
  }
}

void Value::CheckInBounds(std::size_t index) const {
  CheckArrayOrNull();
  if (!GetNative().isValidIndex(index)) {
    throw OutOfBoundsException(index, GetSize(), path_.ToString());
  }
}

}  // namespace formats::json
