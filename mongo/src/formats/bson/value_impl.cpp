#include <formats/bson/value_impl.hpp>

#include <cassert>
#include <cstring>

#include <formats/bson/bson_builder.hpp>
#include <formats/bson/exception.hpp>
#include <formats/bson/wrappers.hpp>
#include <utils/text.hpp>

namespace formats::bson::impl {
namespace {

static constexpr bson_value_t kDefaultBsonValue{BSON_TYPE_EOD, {}, {}};

class IsNullptrVisitor : public boost::static_visitor<bool> {
 public:
  bool operator()(std::nullptr_t) const { return true; }

  template <typename T>
  bool operator()(const T&) const {
    return false;
  }
};

template <typename Variant>
bool IsNullptr(const Variant& variant) {
  return boost::apply_visitor(IsNullptrVisitor{}, variant);
}

class DeepCopyVisitor : public boost::static_visitor<ValueImpl::ParsedValue> {
 public:
  ValueImpl::ParsedValue operator()(std::nullptr_t) const { return nullptr; }

  ValueImpl::ParsedValue operator()(const ParsedDocument& src) const {
    ParsedDocument dest;
    for (const auto& [k, v] : src) {
      dest[k] = std::make_shared<ValueImpl>(*v);
    }
    return dest;
  }

  ValueImpl::ParsedValue operator()(const ParsedArray& src) const {
    ParsedArray dest;
    for (const auto& elem : src) {
      dest.push_back(std::make_shared<ValueImpl>(*elem));
    }
    return dest;
  }
};

void UpdateStringPointers(bson_value_t& value, std::string* storage_ptr) {
  if (!storage_ptr) return;
  auto& str = *storage_ptr;

  if (value.value_type == BSON_TYPE_UTF8) {
    value.value.v_utf8.str = &str[0];
    value.value.v_utf8.len = str.size();
  } else if (value.value_type == BSON_TYPE_BINARY) {
    value.value.v_binary.data = reinterpret_cast<uint8_t*>(&str[0]);
    value.value.v_binary.data_len = str.size();
  }
}

template <typename Func>
void ForEachValue(const uint8_t* data, size_t length, const Path& path,
                  Func&& func) {
  bson_iter_t it;
  if (!bson_iter_init_from_data(&it, data, length)) {
    throw ParseException("malformed BSON at " + PathToString(path));
  }
  while (bson_iter_next(&it)) {
    func(&it);
  }
}

bool operator==(const bson_value_t& lhs, const bson_value_t& rhs) {
  if (lhs.value_type == BSON_TYPE_EOD || rhs.value_type == BSON_TYPE_EOD)
    return false;
  if (lhs.value_type != rhs.value_type) return false;

  switch (lhs.value_type) {
    case BSON_TYPE_OID:
      return bson_oid_equal(&lhs.value.v_oid, &rhs.value.v_oid);
    case BSON_TYPE_INT64:
      return lhs.value.v_int64 == rhs.value.v_int64;
    case BSON_TYPE_INT32:
      return lhs.value.v_int32 == rhs.value.v_int32;
    case BSON_TYPE_DOUBLE:
      return lhs.value.v_double == rhs.value.v_double;
    case BSON_TYPE_BOOL:
      return lhs.value.v_bool == rhs.value.v_bool;
    case BSON_TYPE_DATE_TIME:
      return lhs.value.v_datetime == rhs.value.v_datetime;
    case BSON_TYPE_TIMESTAMP:
      return lhs.value.v_timestamp.timestamp ==
                 rhs.value.v_timestamp.timestamp &&
             lhs.value.v_timestamp.increment == rhs.value.v_timestamp.increment;
    case BSON_TYPE_UTF8:
      return lhs.value.v_utf8.len == rhs.value.v_utf8.len &&
             !std::memcmp(lhs.value.v_utf8.str, rhs.value.v_utf8.str,
                          lhs.value.v_utf8.len);
    case BSON_TYPE_BINARY:
      // subtype is ignored at this time
      return lhs.value.v_binary.data_len == rhs.value.v_binary.data_len &&
             !std::memcmp(lhs.value.v_binary.data, rhs.value.v_binary.data,
                          lhs.value.v_binary.data_len);
    case BSON_TYPE_REGEX:
      return !std::strcmp(lhs.value.v_regex.options,
                          rhs.value.v_regex.options) &&
             !std::strcmp(lhs.value.v_regex.regex, rhs.value.v_regex.regex);
    case BSON_TYPE_DBPOINTER:
      return lhs.value.v_dbpointer.collection_len ==
                 rhs.value.v_dbpointer.collection_len &&
             bson_oid_equal(&lhs.value.v_dbpointer.oid,
                            &rhs.value.v_dbpointer.oid) &&
             !std::memcmp(lhs.value.v_dbpointer.collection,
                          rhs.value.v_dbpointer.collection,
                          lhs.value.v_dbpointer.collection_len);
    case BSON_TYPE_CODE:
      return lhs.value.v_code.code_len == rhs.value.v_code.code_len &&
             !std::memcmp(lhs.value.v_code.code, rhs.value.v_code.code,
                          lhs.value.v_code.code_len);
    case BSON_TYPE_CODEWSCOPE:
      return lhs.value.v_codewscope.code_len ==
                 rhs.value.v_codewscope.code_len &&
             lhs.value.v_codewscope.scope_len ==
                 rhs.value.v_codewscope.scope_len &&
             !std::memcmp(lhs.value.v_codewscope.code,
                          rhs.value.v_codewscope.code,
                          lhs.value.v_codewscope.code_len) &&
             !std::memcmp(lhs.value.v_codewscope.scope_data,
                          rhs.value.v_codewscope.scope_data,
                          lhs.value.v_codewscope.scope_len);
    case BSON_TYPE_SYMBOL:
      return lhs.value.v_symbol.len == rhs.value.v_symbol.len &&
             !std::memcmp(lhs.value.v_symbol.symbol, rhs.value.v_symbol.symbol,
                          lhs.value.v_symbol.len);
    case BSON_TYPE_DECIMAL128:
      return lhs.value.v_decimal128.high == rhs.value.v_decimal128.high &&
             lhs.value.v_decimal128.low == rhs.value.v_decimal128.low;

    case BSON_TYPE_UNDEFINED:
    case BSON_TYPE_NULL:
    case BSON_TYPE_MAXKEY:
    case BSON_TYPE_MINKEY:
      return true;

    default:
      assert(!"Unexpected value_type in bson_value comparison");
      return false;
  }
}

}  // namespace

class ValueImpl::EmplaceEnabler {};

ValueImpl::ValueImpl() : bson_value_(kDefaultBsonValue) {}

ValueImpl::ValueImpl(std::nullptr_t) : bson_value_(kDefaultBsonValue) {
  bson_value_.value_type = BSON_TYPE_NULL;
}

ValueImpl::ValueImpl(BsonHolder bson, DocumentKind kind)
    : storage_(std::move(bson)), bson_value_(kDefaultBsonValue) {
  switch (kind) {
    case DocumentKind::kDocument:
      bson_value_.value_type = BSON_TYPE_DOCUMENT;
      break;
    case DocumentKind::kArray:
      bson_value_.value_type = BSON_TYPE_ARRAY;
      break;
  }
  const auto& stored_bson = GetBson();
  bson_value_.value.v_doc.data =
      const_cast<uint8_t*>(bson_get_data(stored_bson.get()));
  bson_value_.value.v_doc.data_len = stored_bson->len;
}

ValueImpl::ValueImpl(bool value) : ValueImpl() {
  bson_value_.value_type = BSON_TYPE_BOOL;
  bson_value_.value.v_bool = value;
}

ValueImpl::ValueImpl(int32_t value) : ValueImpl() {
  bson_value_.value_type = BSON_TYPE_INT32;
  bson_value_.value.v_int32 = value;
}

ValueImpl::ValueImpl(int64_t value) : ValueImpl() {
  bson_value_.value_type = BSON_TYPE_INT64;
  bson_value_.value.v_int64 = value;
}

ValueImpl::ValueImpl(double value) : ValueImpl() {
  bson_value_.value_type = BSON_TYPE_DOUBLE;
  bson_value_.value.v_double = value;
}

ValueImpl::ValueImpl(const char* value) : ValueImpl(std::string(value)) {}

ValueImpl::ValueImpl(std::string value) : bson_value_(kDefaultBsonValue) {
  if (!utils::text::IsUtf8(value)) {
    throw BsonException("BSON strings must be valid UTF-8");
  }
  storage_ = std::move(value);
  bson_value_.value_type = BSON_TYPE_UTF8;
  UpdateStringPointers(bson_value_, boost::get<std::string>(&storage_));
}

ValueImpl::ValueImpl(const std::chrono::system_clock::time_point& value)
    : ValueImpl() {
  int64_t ms_since_epoch =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          value.time_since_epoch())
          .count();
  bson_value_.value_type = BSON_TYPE_DATE_TIME;
  bson_value_.value.v_datetime = ms_since_epoch;
}

ValueImpl::ValueImpl(const Oid& value) : ValueImpl() {
  bson_value_.value_type = BSON_TYPE_OID;
  bson_value_.value.v_oid = *value.GetNative();
}

ValueImpl::ValueImpl(Binary value)
    : storage_(std::move(value).ToString()), bson_value_(kDefaultBsonValue) {
  bson_value_.value_type = BSON_TYPE_BINARY;
  bson_value_.value.v_binary.subtype = BSON_SUBTYPE_BINARY;
  UpdateStringPointers(bson_value_, boost::get<std::string>(&storage_));
}

ValueImpl::ValueImpl(const Decimal128& value) : ValueImpl() {
  bson_value_.value_type = BSON_TYPE_DECIMAL128;
  bson_value_.value.v_decimal128 = *value.GetNative();
}

ValueImpl::ValueImpl(MinKey) : ValueImpl() {
  bson_value_.value_type = BSON_TYPE_MINKEY;
}

ValueImpl::ValueImpl(MaxKey) : ValueImpl() {
  bson_value_.value_type = BSON_TYPE_MAXKEY;
}

ValueImpl::ValueImpl(EmplaceEnabler, Storage storage, Path path,
                     const bson_value_t& bson_value, uint32_t index)
    : storage_(std::move(storage)),
      path_(std::move(path)),
      bson_value_(bson_value) {
  path_.push_back('[' + std::to_string(index) + ']');
  UpdateStringPointers(bson_value_, boost::get<std::string>(&storage_));
}

ValueImpl::ValueImpl(EmplaceEnabler, Storage storage, Path path,
                     const bson_value_t& bson_value, std::string key)
    : storage_(std::move(storage)),
      path_(std::move(path)),
      bson_value_(bson_value) {
  path_.push_back(std::move(key));
  UpdateStringPointers(bson_value_, boost::get<std::string>(&storage_));
}

ValueImpl::ValueImpl(const ValueImpl& other)
    : storage_(other.storage_),
      bson_value_(other.bson_value_),
      parsed_value_(
          boost::apply_visitor(DeepCopyVisitor{}, other.parsed_value_)) {
  UpdateStringPointers(bson_value_, boost::get<std::string>(&storage_));
}

ValueImpl::ValueImpl(ValueImpl&& other) noexcept { *this = std::move(other); }

ValueImpl& ValueImpl::operator=(const ValueImpl& rhs) {
  if (this == &rhs) return *this;

  ValueImpl tmp(rhs);
  return *this = std::move(tmp);
}

ValueImpl& ValueImpl::operator=(ValueImpl&& rhs) noexcept {
  if (this == &rhs) return *this;

  storage_ = std::move(rhs.storage_);
  bson_value_ = rhs.bson_value_;
  UpdateStringPointers(bson_value_, boost::get<std::string>(&storage_));
  parsed_value_ = std::move(rhs.parsed_value_);
  return *this;
}

bool ValueImpl::IsStorageOwner() const {
  class Visitor : public boost::static_visitor<bool> {
   public:
    Visitor(const ValueImpl& value) : value_(value) {}

    bool operator()(std::nullptr_t) const { return true; }

    bool operator()(const BsonHolder& bson) const {
      return value_.IsDocument() &&
             value_.bson_value_.value.v_doc.data == bson_get_data(bson.get());
    }

    bool operator()(const std::string&) const { return true; }

   private:
    const ValueImpl& value_;
  };

  CheckNotMissing();
  return boost::apply_visitor(Visitor(*this), storage_);
}

bson_type_t ValueImpl::Type() const { return bson_value_.value_type; }

ValueImplPtr ValueImpl::operator[](const std::string& name) {
  if (!IsMissing() && !IsNull()) {
    CheckIsDocument();
    EnsureParsed();
    const auto& parsed_doc = boost::get<ParsedDocument>(parsed_value_);
    auto it = parsed_doc.find(name);
    if (it != parsed_doc.end()) return it->second;
  }
  return std::make_shared<ValueImpl>(EmplaceEnabler{}, nullptr, path_,
                                     kDefaultBsonValue, name);
}

ValueImplPtr ValueImpl::operator[](uint32_t idx) {
  if (IsNull()) throw OutOfBoundsException(idx, 0, GetPath());

  CheckIsArray();
  EnsureParsed();
  CheckInBounds(idx);
  return boost::get<ParsedArray>(parsed_value_)[idx];
}

bool ValueImpl::HasMember(const std::string& name) {
  if (IsMissing() || IsNull()) return false;

  CheckIsDocument();
  EnsureParsed();
  return boost::get<ParsedDocument>(parsed_value_).count(name);
}

ValueImplPtr ValueImpl::GetOrInsert(const std::string& key) {
  if (IsMissing() || IsNull()) {
    parsed_value_ = ParsedDocument();
    bson_value_.value_type = BSON_TYPE_DOCUMENT;
  }
  CheckIsDocument();
  EnsureParsed();
  return boost::get<ParsedDocument>(parsed_value_)
      .emplace(key, std::make_shared<ValueImpl>(EmplaceEnabler{}, nullptr,
                                                path_, kDefaultBsonValue, key))
      .first->second;
}

void ValueImpl::Resize(uint32_t new_size) {
  if (IsMissing() || IsNull()) {
    parsed_value_ = ParsedArray();
    bson_value_.value_type = BSON_TYPE_ARRAY;
  }
  CheckIsArray();
  EnsureParsed();
  auto& parsed_array = boost::get<ParsedArray>(parsed_value_);
  const auto old_size = parsed_array.size();
  parsed_array.resize(new_size);
  for (auto size = old_size; size < new_size; ++size) {
    parsed_array[size] = std::make_shared<ValueImpl>(
        EmplaceEnabler{}, nullptr, path_, kDefaultBsonValue, size);
  }
}

void ValueImpl::PushBack(ValueImplPtr impl) {
  if (IsMissing() || IsNull()) Resize(0);
  EnsureParsed();
  boost::get<ParsedArray>(parsed_value_).push_back(std::move(impl));
}

uint32_t ValueImpl::GetSize() const {
  class Visitor : public boost::static_visitor<uint32_t> {
   public:
    Visitor(const ValueImpl& value) : value_(value) {}

    uint32_t operator()(std::nullptr_t) const {
      uint32_t size = 0;
      ForEachValue(value_.bson_value_.value.v_doc.data,
                   value_.bson_value_.value.v_doc.data_len, value_.path_,
                   [&size](bson_iter_t*) { ++size; });
      return size;
    }

    uint32_t operator()(const ParsedDocument& doc) const { return doc.size(); }
    uint32_t operator()(const ParsedArray& array) const { return array.size(); }

   private:
    const ValueImpl& value_;
  };

  if (IsNull()) return 0;
  CheckIsDocumentOrArray();
  return boost::apply_visitor(Visitor(*this), parsed_value_);
}

std::string ValueImpl::GetPath() const { return PathToString(path_); }

ValueImpl::Iterator ValueImpl::Begin() {
  class Visitor : public boost::static_visitor<ValueImpl::Iterator> {
   public:
    result_type operator()(std::nullptr_t) const {
      assert(false);
      return {};
    }

    result_type operator()(const ParsedArray& arr) const {
      return arr.cbegin();
    }
    result_type operator()(const ParsedDocument& doc) const {
      return doc.cbegin();
    }
  };
  if (IsNull()) return {};
  CheckIsDocumentOrArray();
  EnsureParsed();
  return boost::apply_visitor(Visitor{}, parsed_value_);
}

ValueImpl::Iterator ValueImpl::End() {
  class Visitor : public boost::static_visitor<ValueImpl::Iterator> {
   public:
    result_type operator()(std::nullptr_t) const {
      assert(false);
      return {};
    }

    result_type operator()(const ParsedArray& arr) const { return arr.cend(); }
    result_type operator()(const ParsedDocument& doc) const {
      return doc.cend();
    }
  };
  if (IsNull()) return {};
  CheckIsDocumentOrArray();
  EnsureParsed();
  return boost::apply_visitor(Visitor{}, parsed_value_);
}

bool ValueImpl::IsMissing() const { return Type() == BSON_TYPE_EOD; }
bool ValueImpl::IsNull() const { return Type() == BSON_TYPE_NULL; }
bool ValueImpl::IsArray() const { return Type() == BSON_TYPE_ARRAY; }
bool ValueImpl::IsDocument() const { return Type() == BSON_TYPE_DOCUMENT; }

void ValueImpl::EnsureParsed() {
  CheckNotMissing();
  if (!IsNullptr(parsed_value_)) return;

  switch (bson_value_.value_type) {
    case BSON_TYPE_EOD:
      assert(false);
      break;

    // these types are parsed during bson_value_t construction
    case BSON_TYPE_DOUBLE:
    case BSON_TYPE_UTF8:
    case BSON_TYPE_BINARY:
    case BSON_TYPE_OID:
    case BSON_TYPE_BOOL:
    case BSON_TYPE_DATE_TIME:
    case BSON_TYPE_NULL:
    case BSON_TYPE_INT32:
    case BSON_TYPE_INT64:
    case BSON_TYPE_DECIMAL128:
    case BSON_TYPE_MAXKEY:
    case BSON_TYPE_MINKEY:
      break;

    // those types are not supported, but can be transferred between values
    case BSON_TYPE_UNDEFINED:
    case BSON_TYPE_REGEX:
    case BSON_TYPE_DBPOINTER:
    case BSON_TYPE_CODE:
    case BSON_TYPE_SYMBOL:
    case BSON_TYPE_CODEWSCOPE:
    case BSON_TYPE_TIMESTAMP:
      break;

    case BSON_TYPE_ARRAY: {
      ParsedArray parsed_array;
      ForEachValue(
          bson_value_.value.v_doc.data, bson_value_.value.v_doc.data_len, path_,
          [this, &parsed_array,
           indexer = ArrayIndexer()](bson_iter_t* it) mutable {
            utils::string_view expected_key = indexer.GetKey();
            utils::string_view actual_key(bson_iter_key(it),
                                          bson_iter_key_len(it));
            if (expected_key != actual_key) {
              throw ParseException(
                  "malformed BSON array at " + GetPath() +
                  ": index mismatch, expected=" + std::string(expected_key) +
                  ", got=" + std::string(actual_key));
            }

            const bson_value_t* iter_value = bson_iter_value(it);
            if (!iter_value) {
              throw ParseException("malformed BSON element at " + GetPath() +
                                   '[' + std::string(expected_key) + ']');
            }
            parsed_array.push_back(
                std::make_shared<ValueImpl>(EmplaceEnabler{}, storage_, path_,
                                            *iter_value, indexer.Index()));
            indexer.Advance();
          });
      parsed_value_ = std::move(parsed_array);
      break;
    }
    case BSON_TYPE_DOCUMENT: {
      ParsedDocument parsed_doc;
      ForEachValue(
          bson_value_.value.v_doc.data, bson_value_.value.v_doc.data_len, path_,
          [this, &parsed_doc](bson_iter_t* it) {
            utils::string_view key(bson_iter_key(it), bson_iter_key_len(it));
            const bson_value_t* iter_value = bson_iter_value(it);
            if (!iter_value) {
              throw ParseException("malformed BSON element at " + GetPath() +
                                   '.' + std::string(key));
            }
            auto [parsed_it, is_new] = parsed_doc.emplace(
                std::string(key),
                std::make_shared<ValueImpl>(EmplaceEnabler{}, storage_, path_,
                                            *iter_value, std::string(key)));
            if (!is_new) {
              throw ParseException("duplicate key '" + std::string(key) +
                                   "' at " + GetPath());
            }
          });
      parsed_value_ = std::move(parsed_doc);
      break;
    }
  }
}

void ValueImpl::SyncBsonValue() {
  // either primitive type or was never touched
  if (IsNullptr(parsed_value_)) return;

  assert(IsDocument() || IsArray());

  if (IsDocument()) {
    *this = ValueImpl(BsonBuilder(*this).Extract(), DocumentKind::kDocument);
  } else if (IsArray()) {
    *this = ValueImpl(BsonBuilder(*this).Extract(), DocumentKind::kArray);
  }
}

bool ValueImpl::operator==(ValueImpl& rhs) {
  CheckNotMissing();
  rhs.CheckNotMissing();
  if (Type() != rhs.Type()) return false;

  EnsureParsed();
  rhs.EnsureParsed();
  assert(parsed_value_.which() == rhs.parsed_value_.which());

  class Visitor : public boost::static_visitor<bool> {
   public:
    Visitor(const ValueImpl& lhs, const ValueImpl& rhs)
        : lhs_(lhs), rhs_(rhs) {}

    bool operator()(std::nullptr_t) const {
      return lhs_.bson_value_ == rhs_.bson_value_;
    }

    bool operator()(const ParsedDocument&) const {
      const auto& lhs_doc = boost::get<ParsedDocument>(lhs_.parsed_value_);
      const auto& rhs_doc = boost::get<ParsedDocument>(rhs_.parsed_value_);
      if (lhs_doc.size() != rhs_doc.size()) return false;
      for (const auto& [k, v] : lhs_doc) {
        auto rhs_it = rhs_doc.find(k);
        if (rhs_it == rhs_doc.end() || *rhs_it->second != *v) return false;
      }
      return true;
    }

    bool operator()(const ParsedArray&) const {
      const auto& lhs_arr = boost::get<ParsedArray>(lhs_.parsed_value_);
      const auto& rhs_arr = boost::get<ParsedArray>(rhs_.parsed_value_);
      if (lhs_arr.size() != rhs_arr.size()) return false;
      for (size_t i = 0; i < lhs_arr.size(); ++i) {
        if (*lhs_arr[i] != *rhs_arr[i]) return false;
      }
      return true;
    }

   private:
    const ValueImpl& lhs_;
    const ValueImpl& rhs_;
  };
  return boost::apply_visitor(Visitor(*this, rhs), parsed_value_);
}

bool ValueImpl::operator!=(ValueImpl& rhs) { return !(*this == rhs); }

void ValueImpl::CheckNotMissing() const {
  if (Type() == BSON_TYPE_EOD) {
    throw MemberMissingException(GetPath());
  }
}

void ValueImpl::CheckIsDocumentOrArray() const {
  CheckNotMissing();
  if (!IsDocument() && !IsArray()) {
    throw TypeMismatchException(bson_value_.value_type, BSON_TYPE_DOCUMENT,
                                GetPath());
  }
}

void ValueImpl::CheckIsDocument() const {
  CheckNotMissing();
  if (!IsDocument()) {
    throw TypeMismatchException(bson_value_.value_type, BSON_TYPE_DOCUMENT,
                                GetPath());
  }
}

void ValueImpl::CheckIsArray() const {
  CheckNotMissing();
  if (!IsArray()) {
    throw TypeMismatchException(bson_value_.value_type, BSON_TYPE_ARRAY,
                                GetPath());
  }
}

void ValueImpl::CheckInBounds(uint32_t idx) const {
  CheckIsArray();
  const auto size = GetSize();
  if (idx >= size) {
    throw OutOfBoundsException(idx, size, GetPath());
  }
}

}  // namespace formats::bson::impl
