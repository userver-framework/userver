#include <userver/formats/json/string_builder.hpp>

#include <cmath>
#include <stdexcept>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <formats/common/validations.hpp>
#include <formats/json/impl/accept.hpp>
#include <userver/formats/json/impl/types.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

struct StringBuilder::Impl {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer{buffer};

  Impl() = default;
};

StringBuilder::StringBuilder() = default;

StringBuilder::~StringBuilder() = default;

std::string_view StringBuilder::GetStringView() const {
  const auto& buffer = impl_->buffer;
  return {buffer.GetString(), buffer.GetLength()};
}

std::string StringBuilder::GetString() const {
  return std::string{GetStringView()};
}

void StringBuilder::WriteNull() { impl_->writer.Null(); }

void StringBuilder::WriteString(std::string_view value) {
  impl_->writer.String(value.data(), value.size());
}

void StringBuilder::WriteBool(bool value) { impl_->writer.Bool(value); }

void StringBuilder::WriteInt64(int64_t value) { impl_->writer.Int64(value); }

void StringBuilder::WriteUInt64(uint64_t value) { impl_->writer.Uint64(value); }

void StringBuilder::WriteDouble(double value) {
  formats::common::ValidateFloat<std::runtime_error>(value);
  impl_->writer.Double(value);
}

void StringBuilder::Key(std::string_view sw) {
  impl_->writer.Key(sw.data(), sw.size());
}

void StringBuilder::WriteRawString(std::string_view value) {
  impl_->writer.RawValue(value.data(), value.size(), {});
}

void StringBuilder::WriteValue(const formats::json::Value& value) {
  formats::json::AcceptNoRecursion(value.GetNative(), impl_->writer);
}

void WriteToStream(bool value, StringBuilder& sw) { sw.WriteBool(value); }

void WriteToStream(long long value, StringBuilder& sw) { sw.WriteInt64(value); }

void WriteToStream(unsigned long long value, StringBuilder& sw) {
  sw.WriteUInt64(value);
}

void WriteToStream(int value, StringBuilder& sw) { sw.WriteInt64(value); }

void WriteToStream(unsigned value, StringBuilder& sw) { sw.WriteUInt64(value); }

void WriteToStream(long value, StringBuilder& sw) { sw.WriteInt64(value); }

void WriteToStream(unsigned long value, StringBuilder& sw) {
  sw.WriteUInt64(value);
}

void WriteToStream(double value, StringBuilder& sw) { sw.WriteDouble(value); }

void WriteToStream(const char* value, StringBuilder& sw) {
  WriteToStream(std::string_view{value}, sw);
}

void WriteToStream(std::string_view value, StringBuilder& sw) {
  sw.WriteString(value);
}

void WriteToStream(const formats::json::Value& value, StringBuilder& sw) {
  sw.WriteValue(value);
}

void WriteToStream(const std::string& value, StringBuilder& sw) {
  WriteToStream(std::string_view{value}, sw);
}

void WriteToStream(std::chrono::system_clock::time_point tp,
                   StringBuilder& sw) {
  WriteToStream(
      utils::datetime::Timestring(tp, "UTC", utils::datetime::kRfc3339Format),
      sw);
}

StringBuilder::ObjectGuard::ObjectGuard(StringBuilder& sw) : sw_(sw) {
  sw_.impl_->writer.StartObject();
}

StringBuilder::ObjectGuard::~ObjectGuard() { sw_.impl_->writer.EndObject(); }

StringBuilder::ArrayGuard::ArrayGuard(StringBuilder& sw) : sw_(sw) {
  sw_.impl_->writer.StartArray();
}

StringBuilder::ArrayGuard::~ArrayGuard() { sw_.impl_->writer.EndArray(); }

}  // namespace formats::json

USERVER_NAMESPACE_END
