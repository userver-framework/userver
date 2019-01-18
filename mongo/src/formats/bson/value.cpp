#include <formats/bson/value.hpp>

#include <boost/numeric/conversion/cast.hpp>

#include <formats/bson/bson_builder.hpp>
#include <formats/bson/document.hpp>
#include <formats/bson/value_impl.hpp>
#include <formats/bson/wrappers.hpp>

namespace formats::bson {

Value::Value() : impl_(std::make_shared<impl::ValueImpl>(nullptr)) {}

Value::Value(impl::ValueImplPtr impl) : impl_(std::move(impl)) {}

Value Value::operator[](const std::string& name) const {
  return Value((*impl_)[name]);
}

Value Value::operator[](uint32_t idx) const { return Value((*impl_)[idx]); }

bool Value::HasMember(const std::string& name) const {
  return impl_->HasMember(name);
}

Value::const_iterator Value::begin() const { return {*impl_, impl_->Begin()}; }
Value::const_iterator Value::end() const { return {*impl_, impl_->End()}; }

uint32_t Value::GetSize() const { return impl_->GetSize(); }
std::string Value::GetPath() const { return impl_->GetPath(); }

bool Value::operator==(const Value& rhs) const { return *impl_ == *rhs.impl_; }
bool Value::operator!=(const Value& rhs) const { return *impl_ != *rhs.impl_; }

bool Value::IsMissing() const { return impl_->IsMissing(); }
bool Value::IsArray() const { return impl_->IsArray(); }
bool Value::IsDocument() const { return impl_->IsDocument(); }
bool Value::IsNull() const { return impl_->IsNull(); }
bool Value::IsBool() const { return impl_->Type() == BSON_TYPE_BOOL; }
bool Value::IsInt32() const { return impl_->Type() == BSON_TYPE_INT32; }

bool Value::IsInt64() const {
  return impl_->Type() == BSON_TYPE_INT64 || IsInt32();
}

bool Value::IsDouble() const {
  return impl_->Type() == BSON_TYPE_DOUBLE || IsInt64();
}

bool Value::IsString() const { return impl_->Type() == BSON_TYPE_UTF8; }
bool Value::IsDateTime() const { return impl_->Type() == BSON_TYPE_DATE_TIME; }
bool Value::IsOid() const { return impl_->Type() == BSON_TYPE_OID; }
bool Value::IsBinary() const { return impl_->Type() == BSON_TYPE_BINARY; }

bool Value::IsDecimal128() const {
  return impl_->Type() == BSON_TYPE_DECIMAL128;
}

bool Value::IsMinKey() const { return impl_->Type() == BSON_TYPE_MINKEY; }
bool Value::IsMaxKey() const { return impl_->Type() == BSON_TYPE_MAXKEY; }

template <>
bool Value::As<bool>() const {
  CheckNotMissing();
  if (IsBool()) return impl_->GetNative()->value.v_bool;
  throw TypeMismatchException(impl_->Type(), BSON_TYPE_BOOL, GetPath());
}

template <>
int32_t Value::As<int32_t>() const {
  CheckNotMissing();
  if (IsInt32()) return impl_->GetNative()->value.v_int32;
  if (IsInt64()) return boost::numeric_cast<int32_t>(As<int64_t>());
  throw TypeMismatchException(impl_->Type(), BSON_TYPE_INT32, GetPath());
}

template <>
int64_t Value::As<int64_t>() const {
  CheckNotMissing();
  if (IsInt32()) return As<int32_t>();
  if (IsInt64()) return impl_->GetNative()->value.v_int64;
  throw TypeMismatchException(impl_->Type(), BSON_TYPE_INT64, GetPath());
}

template <>
uint64_t Value::As<uint64_t>() const {
  CheckNotMissing();
  if (IsInt32()) return boost::numeric_cast<uint64_t>(As<int32_t>());
  if (IsInt64()) return boost::numeric_cast<uint64_t>(As<int64_t>());
  throw TypeMismatchException(impl_->Type(), BSON_TYPE_INT64, GetPath());
}

template <>
double Value::As<double>() const {
  CheckNotMissing();
  if (IsInt32()) return boost::numeric_cast<double>(As<int32_t>());
  if (IsInt64()) return boost::numeric_cast<double>(As<int64_t>());
  if (IsDouble()) return impl_->GetNative()->value.v_double;
  throw TypeMismatchException(impl_->Type(), BSON_TYPE_DOUBLE, GetPath());
}

template <>
std::string Value::As<std::string>() const {
  CheckNotMissing();
  if (IsString()) {
    const auto& str = impl_->GetNative()->value.v_utf8;
    return {str.str, str.len};
  }
  throw TypeMismatchException(impl_->Type(), BSON_TYPE_UTF8, GetPath());
}

template <>
std::chrono::system_clock::time_point
Value::As<std::chrono::system_clock::time_point>() const {
  CheckNotMissing();
  if (IsDateTime())
    return std::chrono::system_clock::time_point(
        std::chrono::milliseconds(impl_->GetNative()->value.v_datetime));
  throw TypeMismatchException(impl_->Type(), BSON_TYPE_DATE_TIME, GetPath());
}

template <>
Oid Value::As<Oid>() const {
  CheckNotMissing();
  if (IsOid()) return impl_->GetNative()->value.v_oid;
  throw TypeMismatchException(impl_->Type(), BSON_TYPE_OID, GetPath());
}

template <>
Binary Value::As<Binary>() const {
  CheckNotMissing();
  if (IsBinary()) {
    const auto& data = impl_->GetNative()->value.v_binary;
    return Binary(
        std::string(reinterpret_cast<const char*>(data.data), data.data_len));
  }
  throw TypeMismatchException(impl_->Type(), BSON_TYPE_BINARY, GetPath());
}

template <>
Decimal128 Value::As<Decimal128>() const {
  CheckNotMissing();
  if (IsDecimal128()) return impl_->GetNative()->value.v_decimal128;
  throw TypeMismatchException(impl_->Type(), BSON_TYPE_DECIMAL128, GetPath());
}

template <>
Document Value::As<Document>() const {
  CheckNotMissing();
  impl_->CheckIsDocument();
  if (impl_->IsStorageOwner()) {
    return Document(impl_->GetBson());
  } else {
    const auto& bson_value = impl_->GetNative();
    return Document(impl::MutableBson(bson_value->value.v_doc.data,
                                      bson_value->value.v_doc.data_len)
                        .Extract());
  }
}

template <>
bool Value::Convert<bool>() const {
  if (IsMissing() || IsNull()) return false;
  if (IsBool()) return As<bool>();
  if (IsInt32()) return As<int32_t>();
  if (IsInt64()) return As<int64_t>();
  if (IsDouble()) return As<double>();
  if (IsDateTime()) return true;
  if (IsString()) return impl_->GetNative()->value.v_utf8.len;
  if (IsBinary()) return impl_->GetNative()->value.v_binary.data_len;
  if (IsArray() || IsDocument()) return GetSize();

  throw TypeMismatchException(impl_->Type(), BSON_TYPE_BOOL, GetPath());
}

template <>
int32_t Value::Convert<int32_t>() const {
  if (IsMissing() || IsNull()) return 0;
  if (IsBool()) return As<bool>();
  if (IsInt32()) return As<int32_t>();
  if (IsInt64()) return boost::numeric_cast<int32_t>(As<int64_t>());
  if (IsDouble()) return boost::numeric_cast<int32_t>(As<double>());

  throw TypeMismatchException(impl_->Type(), BSON_TYPE_INT32, GetPath());
}

template <>
int64_t Value::Convert<int64_t>() const {
  if (IsMissing() || IsNull()) return 0;
  if (IsBool()) return As<bool>();
  if (IsInt32()) return As<int32_t>();
  if (IsInt64()) return As<int64_t>();
  if (IsDouble()) return boost::numeric_cast<int64_t>(As<double>());
  if (IsDateTime()) return impl_->GetNative()->value.v_datetime;

  throw TypeMismatchException(impl_->Type(), BSON_TYPE_INT64, GetPath());
}

template <>
uint64_t Value::Convert<uint64_t>() const {
  if (IsDouble()) return boost::numeric_cast<uint64_t>(As<double>());
  return boost::numeric_cast<uint64_t>(Convert<int64_t>());
}

template <>
double Value::Convert<double>() const {
  if (IsDouble()) return As<double>();
  return Convert<int64_t>();
}

template <>
std::string Value::Convert<std::string>() const {
  if (IsString()) return As<std::string>();
  if (IsBinary()) return As<Binary>().ToString();

  if (IsMissing() || IsNull()) return {};
  if (IsBool()) return As<bool>() ? "true" : "false";
  if (IsInt64() || IsDateTime()) return std::to_string(Convert<int64_t>());
  if (IsDouble()) return std::to_string(As<double>());
  if (IsOid()) return As<Oid>().ToString();

  throw TypeMismatchException(impl_->Type(), BSON_TYPE_UTF8, GetPath());
}

#ifdef _LIBCPP_VERSION
template <>
long Value::As<long>() const {
#else
template <>
long long Value::As<long long>() const {
#endif
  return As<int64_t>();
}

#ifdef _LIBCPP_VERSION
template <>
unsigned long Value::As<unsigned long>() const {
#else
template <>
unsigned long long Value::As<unsigned long long>() const {
#endif
  return As<uint64_t>();
}

#ifdef _LIBCPP_VERSION
template <>
long Value::Convert<long>() const {
#else
template <>
long long Value::Convert<long long>() const {
#endif
  return Convert<int64_t>();
}

#ifdef _LIBCPP_VERSION
template <>
unsigned long Value::Convert<unsigned long>() const {
#else
template <>
unsigned long long Value::Convert<unsigned long long>() const {
#endif
  return Convert<uint64_t>();
}

void Value::CheckNotMissing() const { impl_->CheckNotMissing(); }

}  // namespace formats::bson
