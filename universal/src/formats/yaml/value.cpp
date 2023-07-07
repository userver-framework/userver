#include <userver/formats/yaml/value.hpp>

#include <string_view>

#include <yaml-cpp/yaml.h>

#include <userver/compiler/demangle.hpp>
#include <userver/formats/yaml/exception.hpp>
#include <userver/formats/yaml/serialize.hpp>

#include "exttypes.hpp"
#include "string_view_support.hpp"

USERVER_NAMESPACE_BEGIN

namespace formats::yaml {

namespace {

// Helper structure for YAML conversions. YAML has built in conversion logic and
// an `T Node::as<T>(U default_value)` function that uses it. We provide
// `IsConvertibleChecker<T>{}` as a `default_value`, and if the
// `IsConvertibleChecker` was converted to T (an implicit conversion operator
// was called), then the conversion failed.
template <class T>
struct IsConvertibleChecker {
  bool& convertible;

  operator T() const {
    convertible = false;
    return {};
  }
};

auto MakeMissingNode() { return YAML::Node{}[0]; }

}  // namespace

Value::Value() noexcept : Value(YAML::Node()) {}

// NOLINTNEXTLINE(performance-noexcept-move-constructor)
Value::Value(Value&&) = default;
Value::Value(const Value&) = default;

// NOLINTNEXTLINE(performance-noexcept-move-constructor)
Value& Value::operator=(Value&& other) {
  value_pimpl_->reset(*other.value_pimpl_);
  path_ = std::move(other.path_);
  return *this;
}

Value& Value::operator=(const Value& other) {
  if (this == &other) return *this;

  value_pimpl_->reset(*other.value_pimpl_);
  path_ = other.path_;
  return *this;
}

Value::~Value() = default;

Value::Value(const YAML::Node& root) noexcept : value_pimpl_(root) {}

Value::Value(Value&& other, std::string path_prefix)
    : value_pimpl_(std::move(other.value_pimpl_)),
      path_(Path::WithPrefix(std::move(path_prefix))) {
  if (!other.path_.IsRoot()) {
    throw PathPrefixException(other.path_.ToStringView(), path_.ToStringView());
  }
}

Value::Value(EmplaceEnabler, const YAML::Node& value,
             const formats::yaml::Path& path, std::string_view key)
    : value_pimpl_(value), path_(path.MakeChildPath(key)) {}

Value::Value(EmplaceEnabler, const YAML::Node& value,
             const formats::yaml::Path& path, size_t index)
    : value_pimpl_(value), path_(path.MakeChildPath(index)) {}

Value Value::MakeNonRoot(const YAML::Node& value,
                         const formats::yaml::Path& path,
                         std::string_view key) {
  return {EmplaceEnabler{}, value, path, key};
}

Value Value::MakeNonRoot(const YAML::Node& value,
                         const formats::yaml::Path& path, size_t index) {
  return {EmplaceEnabler{}, value, path, index};
}

Value Value::operator[](std::string_view key) const {
  if (!IsMissing()) {
    CheckObjectOrNull();
    return MakeNonRoot((*value_pimpl_)[key], path_, key);
  }
  return MakeNonRoot(MakeMissingNode(), path_, key);
}

Value Value::operator[](std::size_t index) const {
  if (!IsMissing()) {
    CheckInBounds(index);
    return MakeNonRoot((*value_pimpl_)[index], path_, index);
  }
  return MakeNonRoot(MakeMissingNode(), path_, index);
}

Value::const_iterator Value::begin() const {
  CheckObjectOrArrayOrNull();
  return {value_pimpl_->begin(), value_pimpl_->IsSequence() ? 0 : -1, path_};
}

Value::const_iterator Value::end() const {
  CheckObjectOrArrayOrNull();
  return {
      value_pimpl_->end(),
      value_pimpl_->IsSequence() ? static_cast<int>(value_pimpl_->size()) : -1,
      path_};
}

bool Value::IsEmpty() const {
  CheckObjectOrArrayOrNull();
  return value_pimpl_->begin() == value_pimpl_->end();
}

std::size_t Value::GetSize() const {
  CheckObjectOrArrayOrNull();
  return value_pimpl_->size();
}

bool Value::operator==(const Value& other) const {
  return GetNative().Scalar() == other.GetNative().Scalar();
}

bool Value::operator!=(const Value& other) const { return !(*this == other); }

bool Value::IsMissing() const { return !*value_pimpl_; }

template <class T>
bool Value::IsConvertible() const {
  if (IsMissing()) {
    return false;
  }

  bool ok = true;
  value_pimpl_->as<T>(IsConvertibleChecker<T>{ok});
  return ok;
}

template <class T>
T Value::ValueAs() const {
  CheckNotMissing();

  bool ok = true;
  auto res = value_pimpl_->as<T>(IsConvertibleChecker<T>{ok});
  if (!ok) {
    throw TypeMismatchException(*value_pimpl_, compiler::GetTypeName<T>(),
                                path_.ToStringView());
  }
  return res;
}

bool Value::IsNull() const noexcept {
  return !IsMissing() && value_pimpl_->IsNull();
}
bool Value::IsBool() const noexcept { return IsConvertible<bool>(); }
bool Value::IsInt() const noexcept { return IsConvertible<int32_t>(); }
bool Value::IsInt64() const noexcept { return IsConvertible<long long>(); }
bool Value::IsUInt64() const noexcept {
  return IsConvertible<unsigned long long>();
}
bool Value::IsDouble() const noexcept { return IsConvertible<double>(); }
bool Value::IsString() const noexcept {
  return !IsMissing() && value_pimpl_->IsScalar();
}
bool Value::IsArray() const noexcept {
  return !IsMissing() && value_pimpl_->IsSequence();
}
bool Value::IsObject() const noexcept {
  return !IsMissing() && value_pimpl_->IsMap();
}

template <>
bool Value::As<bool>() const {
  return ValueAs<bool>();
}

template <>
int64_t Value::As<int64_t>() const {
  return ValueAs<int64_t>();
}

template <>
uint64_t Value::As<uint64_t>() const {
  return ValueAs<uint64_t>();
}

template <>
double Value::As<double>() const {
  return ValueAs<double>();
}

template <>
std::string Value::As<std::string>() const {
  CheckNotMissing();
  CheckString();
  return value_pimpl_->Scalar();
}

bool Value::HasMember(std::string_view key) const {
  if (IsMissing()) return false;
  CheckObjectOrNull();
  return static_cast<bool>((*value_pimpl_)[key]);
}

std::string Value::GetPath() const { return path_.ToString(); }

namespace {

template <class T>
auto GetMark(const T& value) -> decltype(value.Mark()) {
  return value.Mark();
}

template <class T, class... None>
auto GetMark(const T&, None&&...) {
  // Fallback for old versions of yaml-cpp that have
  // no Mark() member function.
  struct FakeMark {
    int line{0};
    int column{0};
  };
  return FakeMark{};
}

}  // namespace

int Value::GetColumn() const {
  return IsMissing() ? -1 : GetMark(GetNative()).column;
}

int Value::GetLine() const {
  return IsMissing() ? -1 : GetMark(GetNative()).line;
}

Value Value::Clone() const {
  Value v;
  *v.value_pimpl_ = YAML::Clone(GetNative());
  return v;
}

bool Value::IsRoot() const noexcept { return path_.IsRoot(); }

bool Value::DebugIsReferencingSameMemory(const Value& other) const {
  return value_pimpl_->is(*other.value_pimpl_);
}

const YAML::Node& Value::GetNative() const {
  CheckNotMissing();
  return *value_pimpl_;
}

YAML::Node& Value::GetNative() {
  CheckNotMissing();
  return *value_pimpl_;
}

// convert internal yaml type to implementation-specific type that
// distinguishes between scalar and object
int Value::GetExtendedType() const {
  return impl::GetExtendedType(GetNative());
}

void Value::CheckNotMissing() const {
  if (IsMissing()) {
    throw MemberMissingException(path_.ToStringView());
  }
}

void Value::CheckArray() const {
  if (!IsArray()) {
    throw TypeMismatchException(GetExtendedType(), impl::arrayValue,
                                path_.ToStringView());
  }
}

void Value::CheckArrayOrNull() const {
  if (!IsArray() && !IsNull()) {
    throw TypeMismatchException(GetExtendedType(), impl::arrayValue,
                                path_.ToStringView());
  }
}

void Value::CheckObjectOrNull() const {
  if (!IsObject() && !IsNull()) {
    throw TypeMismatchException(GetExtendedType(), impl::objectValue,
                                path_.ToStringView());
  }
}

void Value::CheckObject() const {
  if (!IsObject()) {
    throw TypeMismatchException(GetExtendedType(), impl::objectValue,
                                path_.ToStringView());
  }
}

void Value::CheckString() const {
  if (!IsString()) {
    throw TypeMismatchException(GetExtendedType(), impl::scalarValue,
                                path_.ToStringView());
  }
}

void Value::CheckObjectOrArrayOrNull() const {
  if (!IsObject() && !IsArray() && !IsNull()) {
    throw TypeMismatchException(GetExtendedType(), impl::arrayValue,
                                path_.ToStringView());
  }
}

void Value::CheckInBounds(std::size_t index) const {
  CheckArrayOrNull();
  if (!(*value_pimpl_)[index]) {
    throw OutOfBoundsException(index, GetSize(), path_.ToStringView());
  }
}

}  // namespace formats::yaml

namespace formats::literals {

yaml::Value operator"" _yaml(const char* str, size_t len) {
  return yaml::FromString(std::string(str, len));
}

}  // namespace formats::literals

USERVER_NAMESPACE_END
