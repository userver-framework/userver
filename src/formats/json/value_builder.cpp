#include <formats/json/value_builder.hpp>

#include <formats/json/exception.hpp>

namespace formats {
namespace json {

ValueBuilder::ValueBuilder(Type type)
    : value_(std::make_shared<Json::Value>(ToNativeType(type))) {}

ValueBuilder::ValueBuilder(const ValueBuilder& other) { Copy(other); }

ValueBuilder::ValueBuilder(ValueBuilder&& other) { *this = std::move(other); }

ValueBuilder::ValueBuilder(const char* str)
    : value_(std::make_shared<Json::Value>(std::string(str))) {}

ValueBuilder::ValueBuilder(uint64_t t)
    : value_(std::make_shared<Json::Value>(Json::UInt64(t))) {}

ValueBuilder::ValueBuilder(int64_t t)
    : value_(std::make_shared<Json::Value>(Json::Int64(t))) {}

ValueBuilder::ValueBuilder(long long t) : ValueBuilder(int64_t(t)) {}

ValueBuilder& ValueBuilder::operator=(const ValueBuilder& other) {
  Copy(other);
  return *this;
}

ValueBuilder& ValueBuilder::operator=(ValueBuilder&& other) {
  if (other.value_.IsRoot()) {
    value_.GetNative() = std::move(other.value_.GetNative());
  } else {
    Copy(other);
  }
  return *this;
}

ValueBuilder::ValueBuilder(const formats::json::Value& other) {
  // As we have new native object created,
  // we fill it with the copy from other's native object.
  value_.GetNative() = other.GetNative();
}

ValueBuilder::ValueBuilder(formats::json::Value&& other) {
  // As we have new native object created,
  // we fill it with the other's native object.
  value_.GetNative() = std::move(other.GetNative());
}

ValueBuilder::ValueBuilder(const NativeValuePtr& root, const Json::Value& val,
                           const formats::json::Path& path,
                           const std::string& key)
    : value_(root, val, path, key) {}

ValueBuilder::ValueBuilder(const NativeValuePtr& root, const Json::Value& val,
                           const formats::json::Path& path, uint32_t index)
    : value_(root, val, path, index) {}

ValueBuilder ValueBuilder::operator[](const std::string& key) {
  value_.CheckObjectOrNull();
  return {value_.root_, value_.GetNative()[key], value_.path_, key};
}

ValueBuilder ValueBuilder::operator[](uint32_t index) {
  value_.CheckOutOfBounds(index);
  return {value_.root_, value_.GetNative()[index], value_.path_, index};
}

ValueBuilder::iterator ValueBuilder::begin() {
  value_.CheckObjectOrArray();
  return {value_.root_, value_.GetNative().begin(), value_.path_};
}

ValueBuilder::iterator ValueBuilder::end() {
  value_.CheckObjectOrArray();
  return {value_.root_, value_.GetNative().end(), value_.path_};
}

uint32_t ValueBuilder::GetSize() const { return value_.GetSize(); }

void ValueBuilder::Resize(uint32_t size) {
  value_.CheckArrayOrNull();
  value_.GetNative().resize(size);
}

void ValueBuilder::PushBack(ValueBuilder&& bld) {
  value_.CheckArrayOrNull();
  value_.GetNative()[static_cast<int>(value_.GetSize())] =
      std::move(bld.value_.GetNative());
}

formats::json::Value ValueBuilder::ExtractValue() {
  if (!value_.IsRoot()) {
    throw JsonException("Extract should be called only from the root builder");
  }

  // Create underlying native object first,
  // then fill it with actual data and don't forget
  // to keep path (needed for iterators)
  formats::json::Value v;
  v.GetNative() = std::move(value_.GetNative());
  v.path_ = std::move(value_.path_);
  return v;
}

void ValueBuilder::Set(const NativeValuePtr& root, const Json::Value& val,
                       const formats::json::Path& path,
                       const std::string& key) {
  value_.Set(root, val, path, key);
}

void ValueBuilder::Set(const NativeValuePtr& root, const Json::Value& val,
                       const formats::json::Path& path, uint32_t index) {
  value_.Set(root, val, path, index);
}

const Json::Value& ValueBuilder::Get() const { return value_.Get(); }

std::string ValueBuilder::GetPath() const { return value_.GetPath(); }

void ValueBuilder::Copy(const ValueBuilder& other) {
  value_.GetNative() = other.value_.GetNative();
}

}  // namespace json
}  // namespace formats
