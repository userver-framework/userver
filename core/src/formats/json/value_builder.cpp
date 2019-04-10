#include <formats/json/value_builder.hpp>

#include <formats/json/exception.hpp>

#include <json/value.h>

#include <utils/assert.hpp>

namespace formats {
namespace json {

namespace {
Json::ValueType ToNativeType(Type type) {
  switch (type) {
    case Type::kNull:
      return Json::nullValue;
    case Type::kArray:
      return Json::arrayValue;
    case Type::kObject:
      return Json::objectValue;
    default:
      UASSERT_MSG(
          false,
          "No mapping from formats::json::Type to Json::ValueType found");
      return Json::nullValue;
  }
}
}  // namespace

ValueBuilder::ValueBuilder(Type type)
    : value_(std::make_shared<Json::Value>(ToNativeType(type))) {}

ValueBuilder::ValueBuilder(const ValueBuilder& other) {
  Copy(value_.GetNative(), other);
}

// NOLINTNEXTLINE(performance-noexcept-move-constructor)
ValueBuilder::ValueBuilder(ValueBuilder&& other) {
  Move(value_.GetNative(), std::move(other));
}

ValueBuilder::ValueBuilder(bool t) : value_(std::make_shared<Json::Value>(t)) {}

ValueBuilder::ValueBuilder(const char* str)
    : value_(std::make_shared<Json::Value>(std::string(str))) {}

ValueBuilder::ValueBuilder(const std::string& str)
    : value_(std::make_shared<Json::Value>(str)) {}

ValueBuilder::ValueBuilder(int t) : value_(std::make_shared<Json::Value>(t)) {}
ValueBuilder::ValueBuilder(unsigned int t)
    : value_(std::make_shared<Json::Value>(t)) {}
ValueBuilder::ValueBuilder(uint64_t t)
    : value_(std::make_shared<Json::Value>(Json::UInt64(t))) {}
ValueBuilder::ValueBuilder(int64_t t)
    : value_(std::make_shared<Json::Value>(Json::Int64(t))) {}

#ifdef _LIBCPP_VERSION  // In libc++ long long and int64_t are the same
ValueBuilder::ValueBuilder(long t) : ValueBuilder(int64_t(t)) {}
ValueBuilder::ValueBuilder(unsigned long t) : ValueBuilder(uint64_t(t)) {}
#else
ValueBuilder::ValueBuilder(long long t) : ValueBuilder(int64_t(t)) {}
ValueBuilder::ValueBuilder(unsigned long long t) : ValueBuilder(uint64_t(t)) {}
#endif
ValueBuilder::ValueBuilder(float t)
    : value_(std::make_shared<Json::Value>(t)) {}
ValueBuilder::ValueBuilder(double t)
    : value_(std::make_shared<Json::Value>(t)) {}

ValueBuilder& ValueBuilder::operator=(const ValueBuilder& other) {
  Copy(value_.GetNative(), other);
  return *this;
}

// NOLINTNEXTLINE(performance-noexcept-move-constructor)
ValueBuilder& ValueBuilder::operator=(ValueBuilder&& other) {
  Move(value_.GetNative(), std::move(other));
  return *this;
}

ValueBuilder::ValueBuilder(const formats::json::Value& other) {
  // As we have new native object created,
  // we fill it with the copy from other's native object.
  value_.GetNative() = other.GetNative();
}

// NOLINTNEXTLINE(performance-noexcept-move-constructor)
ValueBuilder::ValueBuilder(formats::json::Value&& other) {
  // As we have new native object created,
  // we fill it with the other's native object.
  if (other.IsUniqueReference())
    value_.GetNative() = std::move(other.GetNative());
  else
    value_.GetNative() = other.GetNative();
}

ValueBuilder::ValueBuilder(const NativeValuePtr& root, const Json::Value& val,
                           const formats::json::Path& path,
                           const std::string& key)
    : value_(root, &val, path, key) {}

ValueBuilder::ValueBuilder(const NativeValuePtr& root, const Json::Value& val,
                           const formats::json::Path& path, std::size_t index)
    : value_(root, val, path, index) {}

ValueBuilder ValueBuilder::operator[](const std::string& key) {
  value_.CheckObjectOrNull();
  return {value_.root_, value_.GetNative()[key], value_.path_, key};
}

ValueBuilder ValueBuilder::operator[](std::size_t index) {
  value_.CheckInBounds(index);
  return {value_.root_, value_.GetNative()[static_cast<int>(index)],
          value_.path_, index};
}

ValueBuilder::iterator ValueBuilder::begin() {
  value_.CheckObjectOrArrayOrNull();
  return {value_.root_, value_.GetNative().begin(), value_.path_};
}

ValueBuilder::iterator ValueBuilder::end() {
  value_.CheckObjectOrArrayOrNull();
  return {value_.root_, value_.GetNative().end(), value_.path_};
}

std::size_t ValueBuilder::GetSize() const { return value_.GetSize(); }

void ValueBuilder::Resize(std::size_t size) {
  value_.CheckArrayOrNull();
  value_.GetNative().resize(size);
}

void ValueBuilder::PushBack(ValueBuilder&& bld) {
  value_.CheckArrayOrNull();
  Move(value_.GetNative()[static_cast<int>(value_.GetSize())], std::move(bld));
}

formats::json::Value ValueBuilder::ExtractValue() {
  if (!value_.IsRoot()) {
    throw Exception("Extract should be called only from the root builder");
  }

  // Create underlying native object first,
  // then fill it with actual data and don't forget
  // to keep path (needed for iterators)
  formats::json::Value v;
  v.GetNative() = std::move(value_.GetNative());
  v.path_ = std::move(value_.path_);
  return v;
}

void ValueBuilder::SetNonRoot(const NativeValuePtr& root,
                              const Json::Value& val,
                              const formats::json::Path& path,
                              const std::string& key) {
  value_.SetNonRoot(root, val, path, key);
}

void ValueBuilder::SetNonRoot(const NativeValuePtr& root,
                              const Json::Value& val,
                              const formats::json::Path& path,
                              std::size_t index) {
  value_.SetNonRoot(root, val, path, index);
}

std::string ValueBuilder::GetPath() const { return value_.GetPath(); }

void ValueBuilder::Copy(Json::Value& to, const ValueBuilder& from) {
  to = from.value_.GetNative();
}

void ValueBuilder::Move(Json::Value& to, ValueBuilder&& from) {
  if (from.value_.IsRoot()) {
    to = std::move(from.value_.GetNative());
  } else {
    Copy(to, from);
  }
}

}  // namespace json
}  // namespace formats
