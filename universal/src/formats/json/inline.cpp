#include <userver/formats/json/inline.hpp>

#include <type_traits>

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <userver/formats/json/exception.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime.hpp>

#include <formats/common/validations.hpp>
#include <formats/json/impl/types_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::impl {
namespace {

::rapidjson::CrtAllocator g_allocator;

static_assert(std::is_empty_v<::rapidjson::CrtAllocator>,
              "allocator has no state");

impl::Value WrapStringView(std::string_view key) {
  // GenericValue ctor has an invalid type for size
  impl::Value wrapped;
  wrapped.SetString(rapidjson::StringRef(key.data(), key.size()), g_allocator);
  return wrapped;
}

std::string FormatTimePoint(std::chrono::system_clock::time_point value) {
  return utils::datetime::Timestring(value, "UTC",
                                     utils::datetime::kRfc3339Format);
}

}  // namespace

InlineObjectBuilder::InlineObjectBuilder()
    : json_(VersionedValuePtr::Create(::rapidjson::Type::kObjectType)) {}

formats::json::Value InlineObjectBuilder::DoBuild() {
  return formats::json::Value(std::move(json_));
}

void InlineObjectBuilder::Reserve(size_t size) {
  json_->MemberReserve(size, g_allocator);
}

void InlineObjectBuilder::Append(std::string_view key, std::nullptr_t) {
  json_->AddMember(WrapStringView(key), impl::Value{}, g_allocator);
}

void InlineObjectBuilder::Append(std::string_view key, bool value) {
  json_->AddMember(WrapStringView(key), impl::Value{value}, g_allocator);
}

void InlineObjectBuilder::Append(std::string_view key, int value) {
  json_->AddMember(WrapStringView(key), impl::Value{value}, g_allocator);
}

void InlineObjectBuilder::Append(std::string_view key, unsigned int value) {
  json_->AddMember(WrapStringView(key), impl::Value{value}, g_allocator);
}

void InlineObjectBuilder::Append(std::string_view key, long value) {
  json_->AddMember(WrapStringView(key), impl::Value{std::int64_t{value}},
                   g_allocator);
}

void InlineObjectBuilder::Append(std::string_view key, unsigned long value) {
  json_->AddMember(WrapStringView(key), impl::Value{std::uint64_t{value}},
                   g_allocator);
}

void InlineObjectBuilder::Append(std::string_view key, long long value) {
  json_->AddMember(WrapStringView(key), impl::Value{std::int64_t{value}},
                   g_allocator);
}

void InlineObjectBuilder::Append(std::string_view key,
                                 unsigned long long value) {
  json_->AddMember(WrapStringView(key), impl::Value{std::uint64_t{value}},
                   g_allocator);
}

void InlineObjectBuilder::Append(std::string_view key, double value) {
  formats::common::ValidateFloat<Exception>(value);
  json_->AddMember(WrapStringView(key), impl::Value{value}, g_allocator);
}

void InlineObjectBuilder::Append(std::string_view key, const char* value) {
  Append(key, std::string_view{value});
}

void InlineObjectBuilder::Append(std::string_view key, std::string_view value) {
  json_->AddMember(WrapStringView(key), WrapStringView(value), g_allocator);
}
void InlineObjectBuilder::Append(std::string_view key,
                                 std::chrono::system_clock::time_point value) {
  Append(key, FormatTimePoint(value));
}

void InlineObjectBuilder::Append(std::string_view key,
                                 const formats::json::Value& value) {
  json_->AddMember(WrapStringView(key),
                   impl::Value(value.GetNative(), g_allocator), g_allocator);
}

InlineArrayBuilder::InlineArrayBuilder()
    : json_(VersionedValuePtr::Create(::rapidjson::Type::kArrayType)) {}

formats::json::Value InlineArrayBuilder::Build() {
  return formats::json::Value(std::move(json_));
}

void InlineArrayBuilder::Reserve(size_t size) {
  json_->Reserve(size, g_allocator);
}

void InlineArrayBuilder::Append(std::nullptr_t) {
  json_->PushBack(impl::Value{}, g_allocator);
}

void InlineArrayBuilder::Append(bool value) {
  json_->PushBack(impl::Value{value}, g_allocator);
}

void InlineArrayBuilder::Append(int value) {
  json_->PushBack(impl::Value{value}, g_allocator);
}

void InlineArrayBuilder::Append(unsigned int value) {
  json_->PushBack(impl::Value{value}, g_allocator);
}

void InlineArrayBuilder::Append(long value) {
  json_->PushBack(impl::Value{std::int64_t{value}}, g_allocator);
}

void InlineArrayBuilder::Append(unsigned long value) {
  json_->PushBack(impl::Value{std::uint64_t{value}}, g_allocator);
}

void InlineArrayBuilder::Append(long long value) {
  json_->PushBack(impl::Value{std::int64_t{value}}, g_allocator);
}

void InlineArrayBuilder::Append(unsigned long long value) {
  json_->PushBack(impl::Value{std::uint64_t{value}}, g_allocator);
}

void InlineArrayBuilder::Append(double value) {
  formats::common::ValidateFloat<Exception>(value);
  json_->PushBack(impl::Value{value}, g_allocator);
}

void InlineArrayBuilder::Append(const char* value) {
  Append(std::string_view{value});
}

void InlineArrayBuilder::Append(std::string_view value) {
  json_->PushBack(WrapStringView(value), g_allocator);
}
void InlineArrayBuilder::Append(std::chrono::system_clock::time_point value) {
  Append(FormatTimePoint(value));
}

void InlineArrayBuilder::Append(const formats::json::Value& value) {
  json_->PushBack(impl::Value(value.GetNative(), g_allocator), g_allocator);
}

}  // namespace formats::json::impl

USERVER_NAMESPACE_END
