#include <userver/formats/bson/binary.hpp>

#include <bson/bson.h>

#include <userver/formats/bson/exception.hpp>

#include <formats/bson/wrappers.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson {

namespace {

constexpr bson_validate_flags_t kBsonValidateFlag =
    static_cast<bson_validate_flags_t>(BSON_VALIDATE_UTF8 |
                                       BSON_VALIDATE_UTF8_ALLOW_NULL |
                                       BSON_VALIDATE_EMPTY_KEYS);

// Attempts to use bson_validate_with_error_and_offset if it is available,
// otherwise fallbacks to the other ValidateWithErrorAndOffset function.
template <class Bson>
auto ValidateWithErrorAndOffset(const Bson* bson)
    -> decltype(bson_validate_with_error_and_offset(bson,
                                                    bson_validate_flags_t{},
                                                    nullptr, nullptr)) {
  size_t error_offset = 0;
  bson_error_t validation_error;
  if (!bson_validate_with_error_and_offset(bson, kBsonValidateFlag,
                                           &error_offset, &validation_error)) {
    throw ParseException("malformed BSON near offset ")
        << error_offset << ": " << validation_error.message;
  }
}

// Fallback to this function if bson.h does not
// provide bson_validate_with_error_and_offset
template <class Bson, class... Args>
auto ValidateWithErrorAndOffset(const Bson& bson, const Args&...) {
  bson_error_t validation_error;
  if (!bson_validate_with_error(bson, kBsonValidateFlag, &validation_error)) {
    throw ParseException("malformed BSON: ") << validation_error.message;
  }
}

}  // namespace

Document FromBinaryString(std::string_view binary) {
  impl::MutableBson native(reinterpret_cast<const uint8_t*>(binary.data()),
                           binary.size());

  if (!native.Get()) {
    throw ParseException("malformed BSON: invalid document length");
  }

  ValidateWithErrorAndOffset(native.Get());

  return Document(native.Extract());
}

BsonString ToBinaryString(const formats::bson::Document& doc) {
  return BsonString(doc.GetBson());
}

BsonString::BsonString(impl::BsonHolder impl) : impl_(std::move(impl)) {}

std::string BsonString::ToString() const {
  return std::string(reinterpret_cast<const char*>(Data()), Size());
}

std::string_view BsonString::GetView() const {
  return std::string_view(reinterpret_cast<const char*>(Data()), Size());
}

const uint8_t* BsonString::Data() const { return bson_get_data(impl_.get()); }

size_t BsonString::Size() const { return impl_->len; }

}  // namespace formats::bson

USERVER_NAMESPACE_END
