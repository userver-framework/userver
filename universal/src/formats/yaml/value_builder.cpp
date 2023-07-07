#include <userver/formats/yaml/value_builder.hpp>

#include <yaml-cpp/yaml.h>

#include <formats/common/validations.hpp>
#include <userver/formats/yaml/exception.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::yaml {

namespace {

YAML::NodeType::value ToNative(Type t) {
  switch (t) {
    case Type::kArray:
      return YAML::NodeType::Sequence;
    case Type::kNull:
      return YAML::NodeType::Null;
    case Type::kObject:
      return YAML::NodeType::Map;
  }

  UINVARIANT(false, "Unexpected YAML type");
}

}  // namespace

ValueBuilder::ValueBuilder() : value_(YAML::Node()) {}

ValueBuilder::ValueBuilder(Type type) : value_(YAML::Node(ToNative(type))) {}

ValueBuilder::ValueBuilder(const ValueBuilder& other) { Copy(other); }

// NOLINTNEXTLINE(performance-noexcept-move-constructor)
ValueBuilder::ValueBuilder(ValueBuilder&& other) { Move(std::move(other)); }

ValueBuilder::ValueBuilder(bool t) : value_(YAML::Node(t)) {}

ValueBuilder::ValueBuilder(const char* str) : value_(YAML::Node(str)) {}

ValueBuilder::ValueBuilder(const std::string& str) : value_(YAML::Node(str)) {}

ValueBuilder::ValueBuilder(int t) : value_(YAML::Node(t)) {}

ValueBuilder::ValueBuilder(unsigned int t) : value_(YAML::Node(t)) {}

ValueBuilder::ValueBuilder(long t) : value_(YAML::Node(t)) {}

ValueBuilder::ValueBuilder(unsigned long t) : value_(YAML::Node(t)) {}

ValueBuilder::ValueBuilder(long long t) : value_(YAML::Node(t)) {}

ValueBuilder::ValueBuilder(unsigned long long t) : value_(YAML::Node(t)) {}

ValueBuilder::ValueBuilder(float t)
    : value_(YAML::Node(formats::common::ValidateFloat<Exception>(t))) {}

ValueBuilder::ValueBuilder(double t)
    : value_(YAML::Node(formats::common::ValidateFloat<Exception>(t))) {}

ValueBuilder& ValueBuilder::operator=(const ValueBuilder& other) {
  if (this == &other) return *this;

  Copy(other);
  return *this;
}

// NOLINTNEXTLINE(performance-noexcept-move-constructor)
ValueBuilder& ValueBuilder::operator=(ValueBuilder&& other) {
  Copy(other);
  return *this;
}

ValueBuilder::ValueBuilder(const formats::yaml::Value& other) {
  NodeDataAssign(other);
}

ValueBuilder::ValueBuilder(formats::yaml::Value&& other) {
  NodeDataAssign(other);
}

ValueBuilder::ValueBuilder(EmplaceEnabler, const YAML::Node& value,
                           const formats::yaml::Path& path,
                           const std::string& key)
    : value_(Value::EmplaceEnabler{}, value, path, key) {}

ValueBuilder::ValueBuilder(EmplaceEnabler, const YAML::Node& value,
                           const formats::yaml::Path& path, size_t index)
    : value_(Value::EmplaceEnabler{}, value, path, index) {}

ValueBuilder::ValueBuilder(common::TransferTag, ValueBuilder&& value) noexcept
    : value_(std::move(value.value_)) {}

ValueBuilder ValueBuilder::MakeNonRoot(const YAML::Node& val,
                                       const formats::yaml::Path& path,
                                       const std::string& key) {
  return {EmplaceEnabler{}, val, path, key};
}

ValueBuilder ValueBuilder::MakeNonRoot(const YAML::Node& val,
                                       const formats::yaml::Path& path,
                                       size_t index) {
  return {EmplaceEnabler{}, val, path, index};
}

ValueBuilder ValueBuilder::operator[](const std::string& key) {
  if (value_.IsMissing()) {
    *value_.value_pimpl_ = YAML::Node(YAML::NodeType::value::Map);
  }
  value_.CheckObjectOrNull();
  return MakeNonRoot(value_.GetNative()[key], value_.path_, key);
}

ValueBuilder ValueBuilder::operator[](std::size_t index) {
  value_.CheckInBounds(index);
  return MakeNonRoot(value_.GetNative()[index], value_.path_, index);
}

void ValueBuilder::Remove(const std::string& key) {
  value_.CheckObject();
  value_.GetNative().remove(key);
}

ValueBuilder::iterator ValueBuilder::begin() {
  value_.CheckObjectOrArrayOrNull();
  return {value_.GetNative().begin(), value_.GetNative().IsSequence() ? 0 : -1,
          value_.path_};
}

ValueBuilder::iterator ValueBuilder::end() {
  value_.CheckObjectOrArrayOrNull();
  return {value_.GetNative().end(),
          value_.GetNative().IsSequence()
              ? static_cast<int>(value_.GetNative().size())
              : -1,
          value_.path_};
}

bool ValueBuilder::IsEmpty() const { return value_.IsEmpty(); }

bool ValueBuilder::IsNull() const noexcept { return value_.IsNull(); }

bool ValueBuilder::IsBool() const noexcept { return value_.IsBool(); }

bool ValueBuilder::IsInt() const noexcept { return value_.IsInt(); }

bool ValueBuilder::IsInt64() const noexcept { return value_.IsInt64(); }

bool ValueBuilder::IsUInt64() const noexcept { return value_.IsUInt64(); }

bool ValueBuilder::IsDouble() const noexcept { return value_.IsDouble(); }

bool ValueBuilder::IsString() const noexcept { return value_.IsString(); }

bool ValueBuilder::IsArray() const noexcept { return value_.IsArray(); }

bool ValueBuilder::IsObject() const noexcept { return value_.IsObject(); }

std::size_t ValueBuilder::GetSize() const { return value_.GetSize(); }

bool ValueBuilder::HasMember(const char* key) const {
  return value_.HasMember(key);
}

bool ValueBuilder::HasMember(const std::string& key) const {
  return value_.HasMember(key);
}

void ValueBuilder::Resize(std::size_t size) {
  value_.CheckArrayOrNull();

  auto sequence_length = value_.GetNative().size();
  if (sequence_length == size) {
    return;
  }

  if (sequence_length < size) {
    for (; sequence_length != size; ++sequence_length) {
      value_.GetNative().push_back(YAML::Node{});
    }
  } else {
    // YAML has problems with updating size after the shrink operation, so
    // instead of removing nodes, we copy them into a new one.
    YAML::Node new_node;
    for (std::size_t i = 0; i != size; ++i) {
      new_node[i] = (*value_.value_pimpl_)[i];
    }

    *value_.value_pimpl_ = new_node;
  }
}

void ValueBuilder::PushBack(ValueBuilder&& bld) {
  value_.CheckArrayOrNull();
  value_.GetNative()[static_cast<int>(value_.GetSize())] =
      std::move(bld).value_.GetNative();
}

formats::yaml::Value ValueBuilder::ExtractValue() {
  if (!value_.IsRoot()) {
    throw Exception("Extract should be called only from the root builder");
  }

  // Create underlying native object first,
  // then fill it with actual data and don't forget
  // to keep path (needed for iterators)
  formats::yaml::Value v = value_.Clone();
  *this = ValueBuilder();
  return v;
}

void ValueBuilder::NodeDataAssign(const formats::yaml::Value& from) {
  if (from.IsMissing()) {
    throw MemberMissingException(from.GetPath());
  }

  *value_.value_pimpl_ = YAML::Clone(*from.value_pimpl_);
}

void ValueBuilder::Copy(const ValueBuilder& from) {
  NodeDataAssign(from.value_);
}

void ValueBuilder::Move(ValueBuilder&& from) {
  Copy(from);
  from.value_ = YAML::Node();
}

}  // namespace formats::yaml

USERVER_NAMESPACE_END
