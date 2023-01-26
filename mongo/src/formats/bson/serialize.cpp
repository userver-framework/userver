#include <userver/formats/bson/serialize.hpp>

#include <ostream>

#include <bson/bson.h>

#include <userver/formats/bson/exception.hpp>
#include <userver/logging/log_helper.hpp>
#include <userver/utils/text.hpp>

#include <formats/bson/value_impl.hpp>
#include <formats/bson/wrappers.hpp>

USERVER_NAMESPACE_BEGIN

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
