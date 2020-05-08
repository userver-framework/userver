#include <formats/json/value_builder.hpp>

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <formats/common/validations.hpp>
#include <formats/json/exception.hpp>
#include <utils/assert.hpp>
#include <utils/datetime.hpp>

namespace formats::json {

namespace {
rapidjson::Type ToNativeType(Type type) {
  switch (type) {
    case Type::kNull:
      return rapidjson::kNullType;
    case Type::kArray:
      return rapidjson::kArrayType;
    case Type::kObject:
      return rapidjson::kObjectType;
    default:
      UASSERT_MSG(
          false,
          "No mapping from formats::json::Type to rapidjson::Type found");
      return rapidjson::kNullType;
  }
}

// use c runtime malloc/free for value builders
rapidjson::CrtAllocator g_crt_allocator;

enum class MemberDuplicate {
  kCheck,
  kNoCheck,
};

impl::Value* AddMember(Value& value, impl::Value& native,
                       const std::string& key,
                       MemberDuplicate member_duplicate) {
  value.CheckObjectOrNull();

  impl::Value* newval = nullptr;

  if (native.IsNull()) {
    native.SetObject();
  } else if (member_duplicate == MemberDuplicate::kCheck) {
    auto it = native.FindMember(key);
    newval = it != native.MemberEnd() ? &it->value : nullptr;
  }

  if (newval == nullptr) {
    // create new member if key is not found
    native.AddMember(impl::Value(key, g_crt_allocator), impl::Value(),
                     g_crt_allocator);
    newval = &std::prev(native.MemberEnd())->value;
  }

  return newval;
}

}  // namespace

ValueBuilder::ValueBuilder(Type type)
    : value_(std::make_shared<impl::Value>(ToNativeType(type))) {}

ValueBuilder::ValueBuilder(const ValueBuilder& other) {
  Copy(value_.GetNative(), other);
}

// NOLINTNEXTLINE(performance-noexcept-move-constructor)
ValueBuilder::ValueBuilder(ValueBuilder&& other) {
  Move(value_.GetNative(), std::move(other));
}

ValueBuilder::ValueBuilder(bool t) : value_(std::make_shared<impl::Value>(t)) {}

ValueBuilder::ValueBuilder(const char* str)
    : value_(std::make_shared<impl::Value>(str, g_crt_allocator)) {}

ValueBuilder::ValueBuilder(const std::string& str)
    : value_(std::make_shared<impl::Value>(str, g_crt_allocator)) {}

ValueBuilder::ValueBuilder(int t) : value_(std::make_shared<impl::Value>(t)) {}
ValueBuilder::ValueBuilder(unsigned int t)
    : value_(std::make_shared<impl::Value>(t)) {}
ValueBuilder::ValueBuilder(uint64_t t)
    : value_(std::make_shared<impl::Value>(t)) {}
ValueBuilder::ValueBuilder(int64_t t)
    : value_(std::make_shared<impl::Value>(t)) {}

ValueBuilder::ValueBuilder(float t)
    : value_(std::make_shared<impl::Value>(
          formats::common::validate_float<Exception>(t))) {}
ValueBuilder::ValueBuilder(double t)
    : value_(std::make_shared<impl::Value>(
          formats::common::validate_float<Exception>(t))) {}

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
  value_.GetNative().CopyFrom(other.GetNative(), g_crt_allocator);
}

// NOLINTNEXTLINE(performance-noexcept-move-constructor)
ValueBuilder::ValueBuilder(formats::json::Value&& other) {
  // As we have new native object created,
  // we fill it with the other's native object.
  if (other.IsUniqueReference())
    value_.GetNative() = std::move(other.GetNative());
  else
    // rapidjson uses move semantics in assignment
    value_.GetNative().CopyFrom(other.GetNative(), g_crt_allocator);
}

ValueBuilder::ValueBuilder(EmplaceEnabler, const NativeValuePtr& root,
                           const impl::Value& val, int depth)
    : ValueBuilder(root, val, depth) {}

ValueBuilder::ValueBuilder(const NativeValuePtr& root, const impl::Value& val,
                           int depth)
    : value_(root, &val, depth) {}

ValueBuilder ValueBuilder::operator[](const std::string& key) {
  auto newval =
      AddMember(value_, value_.GetNative(), key, MemberDuplicate::kCheck);
  return {value_.root_, *newval, value_.depth_ + 1};
}

ValueBuilder ValueBuilder::operator[](std::size_t index) {
  value_.CheckInBounds(index);
  return {value_.root_, value_.GetNative()[static_cast<int>(index)],
          value_.depth_ + 1};
}

void ValueBuilder::EmplaceNocheck(const std::string& key, ValueBuilder value) {
  auto newval =
      AddMember(value_, value_.GetNative(), key, MemberDuplicate::kNoCheck);
  ValueBuilder{value_.root_, *newval, value_.depth_ + 1} = std::move(value);
}

void ValueBuilder::Remove(const std::string& key) {
  value_.CheckObject();
  value_.GetNative().RemoveMember(key);
}

ValueBuilder::iterator ValueBuilder::begin() {
  value_.CheckObjectOrArrayOrNull();
  return {value_.root_, &value_.GetNative(), 0, value_.depth_};
}

ValueBuilder::iterator ValueBuilder::end() {
  value_.CheckObjectOrArrayOrNull();
  return {value_.root_, &value_.GetNative(), static_cast<int>(GetSize()),
          value_.depth_};
}

bool ValueBuilder::IsEmpty() const { return value_.IsEmpty(); }

std::size_t ValueBuilder::GetSize() const { return value_.GetSize(); }

void ValueBuilder::Resize(std::size_t size) {
  value_.CheckArrayOrNull();
  auto& native = value_.GetNative();
  if (native.IsNull()) native.SetArray();
  unsigned actual_size = native.Size();
  if (actual_size < size) {
    for (int count = size - actual_size; count != 0; count--)
      native.PushBack(impl::Value{}, g_crt_allocator);
  } else if (actual_size > size) {
    for (int count = actual_size - size; count != 0; count--) native.PopBack();
  }
}

void ValueBuilder::PushBack(ValueBuilder&& bld) {
  value_.CheckArrayOrNull();
  auto& native = value_.GetNative();
  if (native.IsNull()) {
    native.SetArray();
  }

  if (bld.value_.IsRoot()) {
    native.PushBack(bld.value_.GetNative(),
                    g_crt_allocator);  // PushBack is moving value via RawAssign
  } else {
    native.PushBack(impl::Value{}, g_crt_allocator);
    Copy(*std::prev(native.End()), bld);
  }
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
  v.depth_ = value_.depth_;
  value_ = Value{};
  return v;
}

void ValueBuilder::Copy(impl::Value& to, const ValueBuilder& from) {
  to.CopyFrom(from.value_.GetNative(), g_crt_allocator);
}

void ValueBuilder::Move(impl::Value& to, ValueBuilder&& from) {
  if (from.value_.IsRoot()) {
    to = std::move(from.value_.GetNative());
  } else {
    Copy(to, from);
  }
}

Value Serialize(std::chrono::system_clock::time_point tp,
                ::formats::serialize::To<Value>) {
  json::ValueBuilder builder =
      utils::datetime::Timestring(tp, "UTC", utils::datetime::kRfc3339Format);
  return builder.ExtractValue();
}

}  // namespace formats::json
