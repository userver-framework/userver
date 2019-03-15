#include <formats/json/value.hpp>

#include <formats/json/exception.hpp>

namespace formats {
namespace json {

Value::Value() noexcept : value_ptr_(nullptr) {}

Value::Value(NativeValuePtr&& root) noexcept
    : root_(std::move(root)), value_ptr_(root_.get()) {}

Value::Value(const NativeValuePtr& root, const Json::Value* value_ptr,
             const formats::json::Path& path, const std::string& key)
    : root_(root),
      value_ptr_(const_cast<Json::Value*>(value_ptr)),
      path_(path) {
  path_.push_back(key);
}

Value::Value(const NativeValuePtr& root, const Json::Value& val,
             const formats::json::Path& path, uint32_t index)
    : root_(root), value_ptr_(const_cast<Json::Value*>(&val)), path_(path) {
  path_.push_back('[' + std::to_string(index) + ']');
}

Value Value::operator[](const std::string& key) const {
  const Json::Value* child = nullptr;
  if (!IsMissing()) {
    CheckObjectOrNull();
    child = GetNative().find(key.data(), key.data() + key.size());
  }
  return {root_, child, path_, key};
}

Value Value::operator[](uint32_t index) const {
  CheckInBounds(index);
  return {root_, GetNative()[index], path_, index};
}

Value::const_iterator Value::begin() const {
  CheckObjectOrArrayOrNull();
  return {root_, GetNative().begin(), path_};
}

Value::const_iterator Value::end() const {
  CheckObjectOrArrayOrNull();
  return {root_, GetNative().end(), path_};
}

uint32_t Value::GetSize() const {
  CheckObjectOrArrayOrNull();
  return GetNative().size();
}

bool Value::operator==(const Value& other) const {
  return GetNative() == other.GetNative();
}

bool Value::operator!=(const Value& other) const {
  return GetNative() != other.GetNative();
}

bool Value::IsMissing() const { return root_ && !value_ptr_; }

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
                              GetPath());
}

template <>
int32_t Value::As<int32_t>() const {
  CheckNotMissing();
  if (IsInt()) return GetNative().asInt();
  throw TypeMismatchException(GetNative().type(), Json::intValue, GetPath());
}

template <>
int64_t Value::As<int64_t>() const {
  CheckNotMissing();
  if (IsInt64()) return GetNative().asInt64();
  throw TypeMismatchException(GetNative().type(), Json::intValue, GetPath());
}

template <>
uint64_t Value::As<uint64_t>() const {
  CheckNotMissing();
  if (IsUInt64()) return GetNative().asUInt64();
  throw TypeMismatchException(GetNative().type(), Json::uintValue, GetPath());
}

template <>
double Value::As<double>() const {
  CheckNotMissing();
  if (IsDouble()) return GetNative().asDouble();
  throw TypeMismatchException(GetNative().type(), Json::realValue, GetPath());
}

template <>
std::string Value::As<std::string>() const {
  CheckNotMissing();
  if (IsString()) return GetNative().asString();
  throw TypeMismatchException(GetNative().type(), Json::stringValue, GetPath());
}

bool Value::HasMember(const char* key) const {
  return !IsMissing() && GetNative().isMember(key);
}

bool Value::HasMember(const std::string& key) const {
  return !IsMissing() && GetNative().isMember(key);
}

std::string Value::GetPath() const { return PathToString(path_); }

Value Value::Clone() const {
  Value v;
  v.GetNative() = GetNative();
  return v;
}

void Value::Set(const NativeValuePtr& root, const Json::Value& val,
                const formats::json::Path& path, const std::string& key) {
  *this = Value(root, &val, path, key);
}

void Value::Set(const NativeValuePtr& root, const Json::Value& val,
                const formats::json::Path& path, uint32_t index) {
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
void Value::EnsureValid() {
  CheckNotMissing();
  if (!root_) {
    root_ = std::make_shared<Json::Value>();
    value_ptr_ = root_.get();
  }
}

bool Value::IsRoot() const { return root_.get() == value_ptr_; }

bool Value::IsUniqueReference() const { return root_.use_count() == 1; }

const Json::Value& Value::GetNative() const {
  const_cast<Value*>(this)->EnsureValid();
  return *value_ptr_;
}

Json::Value& Value::GetNative() {
  EnsureValid();
  return *value_ptr_;
}

void Value::CheckNotMissing() const {
  if (IsMissing()) {
    throw MemberMissingException(GetPath());
  }
}

void Value::CheckArrayOrNull() const {
  if (!IsArray() && !IsNull()) {
    throw TypeMismatchException(GetNative().type(), Json::arrayValue,
                                GetPath());
  }
}

void Value::CheckObjectOrNull() const {
  if (!IsObject() && !IsNull()) {
    throw TypeMismatchException(GetNative().type(), Json::objectValue,
                                GetPath());
  }
}

void Value::CheckObjectOrArrayOrNull() const {
  if (!IsObject() && !IsArray() && !IsNull()) {
    throw TypeMismatchException(GetNative().type(), Json::objectValue,
                                GetPath());
  }
}

void Value::CheckInBounds(uint32_t index) const {
  CheckArrayOrNull();
  if (!GetNative().isValidIndex(index)) {
    throw OutOfBoundsException(index, GetSize(), GetPath());
  }
}

}  // namespace json
}  // namespace formats
