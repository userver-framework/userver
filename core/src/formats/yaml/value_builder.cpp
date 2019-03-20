#include <formats/yaml/value_builder.hpp>

#include <formats/yaml/exception.hpp>

#include <utils/assert.hpp>

#include <yaml-cpp/yaml.h>

namespace formats {
namespace yaml {

namespace {

YAML::NodeType::value ToNative(Type t) {
  switch (t) {
    case Type::kArray:
      return YAML::NodeType::Sequence;
    case Type::kMissing:
      return YAML::NodeType::Undefined;
    case Type::kNull:
      return YAML::NodeType::Null;
    case Type::kObject:
      return YAML::NodeType::Map;
  }
}

}  // namespace

ValueBuilder::ValueBuilder() : value_(YAML::Node()) {}

ValueBuilder::ValueBuilder(Type type)
    : value_(type == Type::kMissing ? YAML::Node()
                                    : YAML::Node(ToNative(type))) {}

ValueBuilder::ValueBuilder(const ValueBuilder& other) { Copy(other); }

ValueBuilder::ValueBuilder(ValueBuilder&& other) { Move(std::move(other)); }

ValueBuilder::ValueBuilder(bool t) : value_(YAML::Node(t)) {}

ValueBuilder::ValueBuilder(const char* str) : value_(YAML::Node(str)) {}

ValueBuilder::ValueBuilder(const std::string& str) : value_(YAML::Node(str)) {}

ValueBuilder::ValueBuilder(int t) : value_(YAML::Node(t)) {}

ValueBuilder::ValueBuilder(unsigned int t) : value_(YAML::Node(t)) {}

ValueBuilder::ValueBuilder(uint64_t t) : value_(YAML::Node(t)) {}

ValueBuilder::ValueBuilder(int64_t t) : value_(YAML::Node(t)) {}

#ifdef _LIBCPP_VERSION
ValueBuilder::ValueBuilder(long t) : value_(YAML::Node(t)) {}
ValueBuilder::ValueBuilder(unsigned long t) : value_(YAML::Node(t)) {}
#else
ValueBuilder::ValueBuilder(long long t) : value_(YAML::Node(t)) {}
ValueBuilder::ValueBuilder(unsigned long long t) : value_(YAML::Node(t)) {}
#endif

ValueBuilder::ValueBuilder(float t) : value_(YAML::Node(t)) {}

ValueBuilder::ValueBuilder(double t) : value_(YAML::Node(t)) {}

ValueBuilder& ValueBuilder::operator=(const ValueBuilder& other) {
  Copy(other);
  return *this;
}

ValueBuilder& ValueBuilder::operator=(ValueBuilder&& other) {
  Copy(std::move(other));
  return *this;
}

ValueBuilder::ValueBuilder(const formats::yaml::Value& other) {
  NodeDataAssign(other);
}

ValueBuilder::ValueBuilder(formats::yaml::Value&& other) {
  NodeDataAssign(std::move(other));
}

ValueBuilder ValueBuilder::MakeNonRoot(const YAML::Node& val,
                                       const formats::yaml::Path& path,
                                       const std::string& key) {
  ValueBuilder ret;
  ret.value_ = Value::MakeNonRoot(val, path, key);
  return ret;
}

ValueBuilder ValueBuilder::MakeNonRoot(const YAML::Node& val,
                                       const formats::yaml::Path& path,
                                       std::size_t index) {
  ValueBuilder ret;
  ret.value_ = Value::MakeNonRoot(val, path, index);
  return ret;
}

ValueBuilder ValueBuilder::operator[](const std::string& key) {
  value_.CheckObjectOrNull();
  return MakeNonRoot(value_.GetNative()[key], value_.path_, key);
}

ValueBuilder ValueBuilder::operator[](std::size_t index) {
  value_.CheckInBounds(index);
  return MakeNonRoot(value_.GetNative()[index], value_.path_, index);
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

std::size_t ValueBuilder::GetSize() const { return value_.GetSize(); }

void ValueBuilder::Resize(std::size_t size) {
  value_.CheckArrayOrNull();

  auto sequence_lengh = value_.GetNative().size();
  if (sequence_lengh == size) {
    return;
  }

  if (sequence_lengh < size) {
    for (; sequence_lengh != size; ++sequence_lengh) {
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

void ValueBuilder::SetNonRoot(const YAML::Node& val,
                              const formats::yaml::Path& path,
                              const std::string& key) {
  value_.SetNonRoot(val, path, key);
}

void ValueBuilder::SetNonRoot(const YAML::Node& val,
                              const formats::yaml::Path& path,
                              std::size_t index) {
  value_.SetNonRoot(val, path, index);
}

std::string ValueBuilder::GetPath() const { return value_.GetPath(); }

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

}  // namespace yaml
}  // namespace formats
