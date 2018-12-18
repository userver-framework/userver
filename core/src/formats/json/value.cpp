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

const Json::Value& Value::Get() const { return GetNative(); }

Value Value::operator[](const std::string& key) const {
  const Json::Value* child = nullptr;
  if (!isMissing()) {
    CheckObjectOrNull();
    child = GetNative().find(key.data(), key.data() + key.size());
  }
  return {root_, child, path_, key};
}

Value Value::operator[](uint32_t index) const {
  CheckOutOfBounds(index);
  return {root_, GetNative()[index], path_, index};
}

Value::const_iterator Value::begin() const {
  CheckObjectOrArray();
  return {root_, GetNative().begin(), path_};
}

Value::const_iterator Value::end() const {
  CheckObjectOrArray();
  return {root_, GetNative().end(), path_};
}

uint32_t Value::GetSize() const {
  CheckObjectOrArray();
  return GetNative().size();
}

bool Value::operator==(const Value& other) const {
  return GetNative() == other.GetNative();
}

bool Value::operator!=(const Value& other) const {
  return GetNative() != other.GetNative();
}

bool Value::isMissing() const { return root_ && !value_ptr_; }

#define IS_TYPE(type)                              \
  bool Value::is##type() const {                   \
    return !isMissing() && GetNative().is##type(); \
  }

#define AS_TYPE(type, c_type, j_type)                                    \
  c_type Value::as##type() const {                                       \
    if (!is##type()) {                                                   \
      throw TypeMismatchException(GetNative(), Json::j_type, GetPath()); \
    }                                                                    \
    return GetNative().as##type();                                       \
  }

#define IS_AS_TYPE(type, c_type, j_type) \
  IS_TYPE(type)                          \
  AS_TYPE(type, c_type, j_type)

IS_TYPE(Null)
IS_AS_TYPE(Bool, bool, booleanValue)
IS_AS_TYPE(Int, int32_t, intValue)
IS_AS_TYPE(Int64, int64_t, intValue)
IS_AS_TYPE(UInt, uint32_t, uintValue)
IS_AS_TYPE(UInt64, uint64_t, uintValue)
IS_TYPE(Integral)
AS_TYPE(Float, float, realValue)
IS_AS_TYPE(Double, double, realValue)
IS_TYPE(Numeric)
IS_AS_TYPE(String, std::string, stringValue)
IS_TYPE(Array)
IS_TYPE(Object)

bool Value::isFloat() const { return isDouble(); }

#undef IS_AS_TYPE
#undef AS_TYPE
#undef IS_TYPE

bool Value::HasMember(const char* key) const {
  return !isMissing() && GetNative().isMember(key);
}

bool Value::HasMember(const std::string& key) const {
  return !isMissing() && GetNative().isMember(key);
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
  if (!root_) {
    root_ = std::make_shared<Json::Value>();
    value_ptr_ = root_.get();
  } else if (isMissing()) {
    throw MemberMissingException(GetPath());
  }
}

bool Value::IsRoot() const { return root_.get() == value_ptr_; }

bool Value::IsUniqueReference() const { return root_.unique(); }

const Json::Value& Value::GetNative() const {
  const_cast<Value*>(this)->EnsureValid();
  return *value_ptr_;
}

Json::Value& Value::GetNative() {
  EnsureValid();
  return *value_ptr_;
}

void Value::CheckArrayOrNull() const {
  if (!isArray() && !isNull()) {
    throw TypeMismatchException(GetNative(), Json::arrayValue, GetPath());
  }
}

void Value::CheckObjectOrNull() const {
  if (!isObject() && !isNull()) {
    throw TypeMismatchException(GetNative(), Json::objectValue, GetPath());
  }
}

void Value::CheckObjectOrArray() const {
  if (!isObject() && !isArray()) {
    throw TypeMismatchException(GetNative(), Json::objectValue, GetPath());
  }
}

void Value::CheckOutOfBounds(uint32_t index) const {
  if (!GetNative().isValidIndex(index)) {
    throw OutOfBoundsException(index, GetSize(), GetPath());
  }
}

}  // namespace json
}  // namespace formats
