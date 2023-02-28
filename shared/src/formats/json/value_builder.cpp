#include <userver/formats/json/value_builder.hpp>

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <userver/formats/json/exception.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime.hpp>

#include <formats/common/validations.hpp>
#include <formats/json/impl/types_impl.hpp>

USERVER_NAMESPACE_BEGIN

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

::rapidjson::CrtAllocator g_allocator;

}  // namespace

ValueBuilder::ValueBuilder(Type type)
    : value_(impl::VersionedValuePtr::Create(ToNativeType(type))) {}

ValueBuilder::ValueBuilder(const ValueBuilder& other) {
  Copy(value_->GetNative(), other);
}

// NOLINTNEXTLINE(performance-noexcept-move-constructor)
ValueBuilder::ValueBuilder(ValueBuilder&& other) {
  Move(value_->GetNative(), std::move(other));
}

ValueBuilder::ValueBuilder(bool t)
    : value_(impl::VersionedValuePtr::Create(t)) {}

ValueBuilder::ValueBuilder(const char* str)
    : value_(impl::VersionedValuePtr::Create(str, g_allocator)) {}

ValueBuilder::ValueBuilder(const std::string& str)
    : value_(impl::VersionedValuePtr::Create(str, g_allocator)) {}

ValueBuilder::ValueBuilder(std::string_view str)
    : value_(impl::VersionedValuePtr{}) {
  // GenericValue ctor has an invalid type for size
  value_->GetNative().SetString(rapidjson::StringRef(str.data(), str.size()),
                                g_allocator);
}

ValueBuilder::ValueBuilder(int t)
    : value_(impl::VersionedValuePtr::Create(t)) {}

ValueBuilder::ValueBuilder(unsigned int t)
    : value_(impl::VersionedValuePtr::Create(t)) {}

ValueBuilder::ValueBuilder(uint64_t t)
    : value_(impl::VersionedValuePtr::Create(t)) {}

ValueBuilder::ValueBuilder(int64_t t)
    : value_(impl::VersionedValuePtr::Create(t)) {}

ValueBuilder::ValueBuilder(float t)
    : value_(impl::VersionedValuePtr::Create(
          formats::common::ValidateFloat<Exception>(t))) {}

ValueBuilder::ValueBuilder(double t)
    : value_(impl::VersionedValuePtr::Create(
          formats::common::ValidateFloat<Exception>(t))) {}

ValueBuilder& ValueBuilder::operator=(const ValueBuilder& other) {
  if (this == &other) return *this;

  if ((value_->IsArray() || value_->IsObject()) && value_->GetSize() != 0) {
    value_.OnMembersChange();
  }
  Copy(value_->GetNative(), other);
  return *this;
}

// NOLINTNEXTLINE(performance-noexcept-move-constructor)
ValueBuilder& ValueBuilder::operator=(ValueBuilder&& other) {
  if ((value_->IsArray() || value_->IsObject()) && value_->GetSize() != 0) {
    value_.OnMembersChange();
  }
  Move(value_->GetNative(), std::move(other));
  return *this;
}

ValueBuilder::ValueBuilder(const formats::json::Value& other) {
  // As we have new native object created,
  // we fill it with the copy from other's native object.
  value_->GetNative().CopyFrom(other.GetNative(), g_allocator);
}

ValueBuilder::ValueBuilder(formats::json::Value&& other) {
  // As we have new native object created,
  // we fill it with the other's native object.
  if (other.IsUniqueReference())
    value_->GetNative() = std::move(other.GetNative());
  else
    // rapidjson uses move semantics in assignment
    value_->GetNative().CopyFrom(other.GetNative(), g_allocator);
}

ValueBuilder::ValueBuilder(EmplaceEnabler,
                           impl::MutableValueWrapper value) noexcept
    : ValueBuilder(std::move(value)) {}

ValueBuilder::ValueBuilder(impl::MutableValueWrapper value) noexcept
    : value_(std::move(value)) {}

ValueBuilder::ValueBuilder(common::TransferTag, ValueBuilder&& value) noexcept
    : value_(std::move(value.value_)) {}

ValueBuilder ValueBuilder::operator[](std::string key) {
  auto& member = AddMember(key, CheckMemberExists::kYes);
  return ValueBuilder{value_.WrapMember(std::move(key), member)};
}

ValueBuilder ValueBuilder::operator[](std::size_t index) {
  value_->CheckInBounds(index);
  return ValueBuilder{value_.WrapElement(index)};
}

void ValueBuilder::EmplaceNocheck(const std::string& key, ValueBuilder value) {
  Move(AddMember(key, CheckMemberExists::kNo), std::move(value));
}

void ValueBuilder::Remove(const std::string& key) {
  value_->CheckObject();
  if (value_->GetNative().RemoveMember(key)) {
    value_.OnMembersChange();
  }
}

ValueBuilder::iterator ValueBuilder::begin() {
  value_->CheckObjectOrArrayOrNull();
  return iterator{value_, 0};
}

ValueBuilder::iterator ValueBuilder::end() {
  value_->CheckObjectOrArrayOrNull();
  return iterator{value_, static_cast<int>(GetSize())};
}

bool ValueBuilder::IsEmpty() const { return value_->IsEmpty(); }

bool ValueBuilder::IsNull() const noexcept { return value_->IsNull(); }

bool ValueBuilder::IsBool() const noexcept { return value_->IsBool(); }

bool ValueBuilder::IsInt() const noexcept { return value_->IsInt(); }

bool ValueBuilder::IsInt64() const noexcept { return value_->IsInt64(); }

bool ValueBuilder::IsUInt64() const noexcept { return value_->IsUInt64(); }

bool ValueBuilder::IsDouble() const noexcept { return value_->IsDouble(); }

bool ValueBuilder::IsString() const noexcept { return value_->IsString(); }

bool ValueBuilder::IsArray() const noexcept { return value_->IsArray(); }

bool ValueBuilder::IsObject() const noexcept { return value_->IsObject(); }

std::size_t ValueBuilder::GetSize() const { return value_->GetSize(); }

bool ValueBuilder::HasMember(const char* key) const {
  return value_->HasMember(key);
}

bool ValueBuilder::HasMember(const std::string& key) const {
  return value_->HasMember(key);
}

std::string ValueBuilder::GetPath() const { return value_->GetPath(); }

void ValueBuilder::Resize(std::size_t size) {
  value_->CheckArrayOrNull();
  auto& native = value_->GetNative();

  if (native.IsNull()) native.SetArray();

  if (size > native.Capacity()) {
    native.Reserve(size, g_allocator);
    value_.OnMembersChange();
  }

  for (size_t curr_size = native.Size(); curr_size > size; --curr_size) {
    native.PopBack();
  }
  for (size_t curr_size = native.Size(); curr_size < size; ++curr_size) {
    native.PushBack(impl::Value{}, g_allocator);
  }
}

void ValueBuilder::PushBack(ValueBuilder&& bld) {
  value_->CheckArrayOrNull();
  auto& native = value_->GetNative();
  if (native.IsNull()) {
    native.SetArray();
  }

  // notify wrapper when elements capacity (and thus location) changes
  const auto checked_push_back = [this, &native](auto&& value) {
    const auto old_capacity = native.Capacity();
    native.PushBack(value, g_allocator);
    if (old_capacity && old_capacity != native.Capacity()) {
      value_.OnMembersChange();
    }
  };

  if (bld.value_->IsRoot()) {
    // PushBack is moving value via RawAssign
    checked_push_back(bld.value_->GetNative());
  } else {
    checked_push_back(impl::Value{});
    Copy(*std::prev(native.End()), bld);
  }
}

formats::json::Value ValueBuilder::ExtractValue() {
  if (!value_->IsRoot()) {
    throw Exception("Extract should be called only from the root builder");
  }
  // reset to a known good state
  return std::exchange(value_, impl::MutableValueWrapper{}).ExtractValue();
}

void ValueBuilder::Copy(impl::Value& to, const ValueBuilder& from) {
  to.CopyFrom(from.value_->GetNative(), g_allocator);
}

void ValueBuilder::Move(impl::Value& to, ValueBuilder&& from) {
  if (from.value_->IsRoot()) {
    to = std::move(from.value_->GetNative());
  } else {
    Copy(to, from);
  }
}

impl::Value& ValueBuilder::AddMember(const std::string& key,
                                     CheckMemberExists check_exists) {
  value_->CheckObjectOrNull();
  auto& native = value_->GetNative();

  if (native.IsNull()) {
    native.SetObject();
  } else if (check_exists == CheckMemberExists::kYes) {
    auto it = native.FindMember(key);
    if (it != native.MemberEnd()) {
      return it->value;
    }
  }

  // notify wrapper when members capacity (and thus location) changes
  const auto old_capacity = native.MemberCapacity();
  native.AddMember(impl::Value{key, g_allocator}, impl::Value{}, g_allocator);
  if (old_capacity && old_capacity != native.MemberCapacity()) {
    value_.OnMembersChange();
  }

  return std::prev(native.MemberEnd())->value;
}

Value Serialize(std::chrono::system_clock::time_point tp,
                formats::serialize::To<Value>) {
  json::ValueBuilder builder =
      utils::datetime::Timestring(tp, "UTC", utils::datetime::kRfc3339Format);
  return builder.ExtractValue();
}

}  // namespace formats::json

USERVER_NAMESPACE_END
