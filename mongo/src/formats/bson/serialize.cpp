#include <userver/formats/bson/serialize.hpp>

#include <ostream>

#include <bson/bson.h>

#include <userver/formats/bson/exception.hpp>
#include <userver/formats/bson/value_builder.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log_helper.hpp>
#include <userver/utils/text.hpp>

#include <formats/bson/value_impl.hpp>
#include <formats/bson/wrappers.hpp>

#include "conversion_stack.hpp"

USERVER_NAMESPACE_BEGIN

namespace formats::parse {

json::Value Convert(const bson::Value& bson, parse::To<json::Value>) {
  ConversionStack<bson::Value, json::ValueBuilder> conversion_stack(bson);
  while (true) {
    const auto& from = conversion_stack.GetNextFrom();
    if (from.IsBool()) {
      conversion_stack.CastCurrentPrimitive<bool>();
    } else if (from.IsInt32()) {
      conversion_stack.CastCurrentPrimitive<int32_t>();
    } else if (from.IsInt64()) {
      conversion_stack.CastCurrentPrimitive<int64_t>();
    } else if (from.IsDouble()) {
      conversion_stack.CastCurrentPrimitive<double>();
    } else if (from.IsTimestamp()) {
      conversion_stack.SetCurrent(from.As<bson::Timestamp>().GetTimestamp());
    } else if (from.IsString()) {
      conversion_stack.CastCurrentPrimitive<std::string>();
    } else if (from.IsDecimal128()) {
      conversion_stack.SetCurrent(from.As<bson::Decimal128>().ToString());
    } else if (from.IsDateTime()) {
      conversion_stack
          .CastCurrentPrimitive<std::chrono::system_clock::time_point>();
    } else if (from.IsOid()) {
      conversion_stack.SetCurrent(from.As<bson::Oid>().ToString());
    } else if (from.IsBinary()) {
      conversion_stack.SetCurrent(from.As<bson::Binary>().ToString());
    } else if (from.IsMinKey()) {
      conversion_stack.SetCurrent(json::MakeObject("$minKey", 1));
    } else if (from.IsMaxKey()) {
      conversion_stack.SetCurrent(json::MakeObject("$maxKey", 1));
    } else if (from.IsMissing() || from.IsNull()) {
      conversion_stack.SetCurrent(common::Type::kNull);
    } else if (from.IsArray() || from.IsObject()) {
      conversion_stack.ParseNext();
    } else {
      throw bson::ConversionException("Value type is unknown");
    }
    if (auto value = conversion_stack.GetValueIfParsed()) {
      return value.value().ExtractValue();
    }
  }
}

bson::Value Convert(const json::Value& json, parse::To<bson::Value>) {
  ConversionStack<json::Value, bson::ValueBuilder> conversion_stack(json);
  while (true) {
    const auto& from = conversion_stack.GetNextFrom();
    if (from.IsBool()) {
      conversion_stack.CastCurrentPrimitive<bool>();
    } else if (from.IsInt()) {
      conversion_stack.CastCurrentPrimitive<int>();
    } else if (from.IsInt64()) {
      conversion_stack.CastCurrentPrimitive<int64_t>();
    } else if (from.IsUInt64()) {
      conversion_stack.CastCurrentPrimitive<uint64_t>();
    } else if (from.IsDouble()) {
      conversion_stack.CastCurrentPrimitive<double>();
    } else if (from.IsString()) {
      conversion_stack.CastCurrentPrimitive<std::string>();
    } else if (from.IsMissing() || from.IsNull()) {
      conversion_stack.SetCurrent(common::Type::kNull);
    } else if (from.IsArray() || from.IsObject()) {
      conversion_stack.ParseNext();
    } else {
      throw bson::ConversionException("Value type is unknown");
    }
    if (auto value = conversion_stack.GetValueIfParsed()) {
      return value.value().ExtractValue();
    }
  }
}

}  // namespace formats::parse

namespace formats::bson {
namespace impl {

class JsonStringImpl {
 public:
  JsonStringImpl(RawPtr<char>&& data, size_t size)
      : data_(std::move(data)), size_(size) {}

  [[nodiscard]] const char* Data() const { return data_.get(); }
  [[nodiscard]] size_t Size() const { return size_; }

 private:
  RawPtr<char> data_;
  size_t size_;
};

}  // namespace impl

namespace {

JsonString ApplyConversionToString(const impl::BsonHolder& bson,
                                   char* (*conversion)(const bson_t*,
                                                       size_t*)) {
  size_t size = 0;
  const bson_t* native_bson_ptr = bson.get();
  impl::RawPtr<char> data(conversion(native_bson_ptr, &size));
  if (!data) {
    throw ConversionException("Cannot convert BSON to JSON");
  }
  return JsonString(impl::JsonStringImpl(std::move(data), size));
}

impl::BsonHolder DoParseJsonString(std::string_view json) {
  bson_error_t error;
  auto bson = impl::MutableBson::AdoptNative(bson_new_from_json(
      reinterpret_cast<const uint8_t*>(json.data()), json.size(), &error));
  if (!bson.Get()) {
    throw ConversionException("Error parsing BSON from JSON: ")
        << error.message;
  }
  return bson.Extract();
}

char FirstNonWhitespace(std::string_view str) {
  for (char c : str) {
    if (!utils::text::IsAsciiSpace(c)) {
      return c;
    }
  }
  return 0;
}

}  // namespace

Document FromJsonString(std::string_view json) {
  if (FirstNonWhitespace(json) != '{') {
    throw ParseException("Error parsing BSON from JSON: not an object");
  }
  return Document(DoParseJsonString(json));
}

Value ArrayFromJsonString(std::string_view json) {
  if (FirstNonWhitespace(json) != '[') {
    throw ParseException("Error parsing BSON from JSON: not an array");
  }
  auto value_impl = std::make_shared<impl::ValueImpl>(
      DoParseJsonString(json), impl::ValueImpl::DocumentKind::kArray);
  value_impl->EnsureParsed();  // force validation
  return Value(std::move(value_impl));
}

JsonString ToCanonicalJsonString(const formats::bson::Document& doc) {
  return ApplyConversionToString(doc.GetBson(),
                                 &bson_as_canonical_extended_json);
}

JsonString ToRelaxedJsonString(const formats::bson::Document& doc) {
  return ApplyConversionToString(doc.GetBson(), &bson_as_relaxed_extended_json);
}

JsonString ToLegacyJsonString(const formats::bson::Document& doc) {
  return ApplyConversionToString(doc.GetBson(), &bson_as_json);
}

JsonString ToArrayJsonString(const formats::bson::Value& array) {
  return ApplyConversionToString(array.GetInternalArrayDocument().GetBson(),
                                 &bson_array_as_json);
}

JsonString::JsonString(impl::JsonStringImpl&& impl) : impl_(std::move(impl)) {}
JsonString::~JsonString() = default;

std::string JsonString::ToString() const {
  return {impl_->Data(), impl_->Size()};
}

std::string_view JsonString::GetView() const {
  return {impl_->Data(), impl_->Size()};
}

const char* JsonString::Data() const { return impl_->Data(); }
size_t JsonString::Size() const { return impl_->Size(); }

std::ostream& operator<<(std::ostream& os, const JsonString& json) {
  return os.write(json.Data(), json.Size());
}

logging::LogHelper& operator<<(logging::LogHelper& lh, const JsonString& json) {
  lh << json.GetView();
  return lh;
}

logging::LogHelper& operator<<(logging::LogHelper& lh, const Document& bson) {
  lh << ToRelaxedJsonString(bson);
  return lh;
}

}  // namespace formats::bson

USERVER_NAMESPACE_END
