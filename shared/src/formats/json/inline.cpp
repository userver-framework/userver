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

void InlineObjectBuilder::Append(std::string_view key, int32_t value) {
  json_->AddMember(WrapStringView(key), impl::Value{value}, g_allocator);
}

void InlineObjectBuilder::Append(std::string_view key, int64_t value) {
  json_->AddMember(WrapStringView(key), impl::Value{value}, g_allocator);
}

void InlineObjectBuilder::Append(std::string_view key, uint32_t value) {
  json_->AddMember(WrapStringView(key), impl::Value{value}, g_allocator);
}

void InlineObjectBuilder::Append(std::string_view key, uint64_t value) {
  json_->AddMember(WrapStringView(key), impl::Value{value}, g_allocator);
}

// MAC_COMPAT: different typedefs for 64_t on mac
#ifdef __APPLE__
void InlineObjectBuilder::Append(std::string_view key, long value) {
#else
void InlineObjectBuilder::Append(std::string_view key, long long value) {
#endif
  Append(key, int64_t{value});
}

// MAC_COMPAT: different typedefs for 64_t on mac
#ifdef __APPLE__
void InlineObjectBuilder::Append(std::string_view key, unsigned long value) {
#else
void InlineObjectBuilder::Append(std::string_view key,
                                 unsigned long long value) {
#endif
  Append(key, uint64_t{value});
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

void InlineArrayBuilder::Append(int32_t value) {
  json_->PushBack(impl::Value{value}, g_allocator);
}

void InlineArrayBuilder::Append(int64_t value) {
  json_->PushBack(impl::Value{value}, g_allocator);
}

void InlineArrayBuilder::Append(uint64_t value) {
  json_->PushBack(impl::Value{value}, g_allocator);
}

// MAC_COMPAT: different typedefs for 64_t on mac
#ifdef __APPLE__
void InlineArrayBuilder::Append(long value) {
#else
void InlineArrayBuilder::Append(long long value) {
#endif
  Append(int64_t{value});
}

// MAC_COMPAT: different typedefs for 64_t on mac
#ifdef __APPLE__
void InlineArrayBuilder::Append(unsigned long value) {
#else
void InlineArrayBuilder::Append(unsigned long long value) {
#endif
  Append(uint64_t{value});
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
