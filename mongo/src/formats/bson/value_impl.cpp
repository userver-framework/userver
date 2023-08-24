#include <formats/bson/value_impl.hpp>

#include <cstring>

#include <formats/bson/wrappers.hpp>

#include <fmt/format.h>

#include <userver/formats/bson/bson_builder.hpp>
#include <userver/formats/bson/exception.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson::impl {
namespace {

constexpr bson_value_t kDefaultBsonValue{BSON_TYPE_EOD, {}, {}};

void RelaxedSetParsedValue(std::atomic<ValueImpl::ParsedValue*>& parsed_value,
                           ValueImpl::ParsedValue&& value) {
  UASSERT(parsed_value.load(std::memory_order_relaxed) == nullptr);
  using ParsedValue = ValueImpl::ParsedValue;
  parsed_value.store(new ParsedValue(std::move(value)),
                     std::memory_order_relaxed);
}

void AtomicSetParsedValue(
    std::atomic<ValueImpl::ParsedValue*>& parsed_value,
    std::unique_ptr<ValueImpl::ParsedValue>&& desired) noexcept {
  // More than one thread may be reading the bson::Value, so rarely we may have
  // more than one thread at this point.
  //
  // To avoid locking, we allow all of the threads to do the decoding, but only
  // the first succeeds. Concurrent work with bson::Value is very rare, so we
  // assume the overhead is rare and negligible because of that.

  ValueImpl::ParsedValue* expected = nullptr;
  if (parsed_value.compare_exchange_strong(expected, desired.get())) {
    // clang-tidy complains on just `desired.release();`
    [[maybe_unused]] auto* already_owned_by_parsed_value = desired.release();
  }
}

class DeepCopyVisitor {
 public:
  ValueImpl::ParsedValue operator()(const ParsedDocument& src) const {
    ParsedDocument dest;
    dest.reserve(src.size());
    for (const auto& [k, v] : src) {
      dest[k] = std::make_shared<ValueImpl>(*v);
    }
    return dest;
  }

  ValueImpl::ParsedValue operator()(const ParsedArray& src) const {
    ParsedArray dest;
    dest.reserve(src.size());
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
    value.value.v_utf8.str = str.data();
    value.value.v_utf8.len = str.size();
  } else if (value.value_type == BSON_TYPE_BINARY) {
    value.value.v_binary.data = reinterpret_cast<uint8_t*>(str.data());
    value.value.v_binary.data_len = str.size();
  }
}

template <typename Func>
void ForEachValue(const uint8_t* data, size_t length, const Path& path,
                  Func&& func) {
  bson_iter_t it;
  if (!bson_iter_init_from_data(&it, data, length)) {
    throw ParseException(
        fmt::format("malformed BSON at {}", path.ToStringView()));
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
      UASSERT_MSG(false, "Unexpected value_type in bson_value comparison");
      return false;
  }
}

}  // namespace

class ValueImpl::EmplaceEnabler {};

ValueImpl::ValueImpl() : bson_value_(kDefaultBsonValue) {}

ValueImpl::~ValueImpl() { delete parsed_value_.load(); }

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
  const bson_t* native_bson_ptr = stored_bson.get();
  bson_value_.value.v_doc.data =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
      const_cast<uint8_t*>(bson_get_data(native_bson_ptr));
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
  UpdateStringPointers(bson_value_, std::get_if<std::string>(&storage_));
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
  UpdateStringPointers(bson_value_, std::get_if<std::string>(&storage_));
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

ValueImpl::ValueImpl(const Timestamp& value) : ValueImpl() {
  bson_value_.value_type = BSON_TYPE_TIMESTAMP;
  bson_value_.value.v_timestamp.timestamp = value.GetTimestamp();
  bson_value_.value.v_timestamp.increment = value.GetIncrement();
}

ValueImpl::ValueImpl(EmplaceEnabler, Storage storage, const Path& path,
                     const bson_value_t& bson_value,
                     Value::DuplicateFieldsPolicy duplicate_fields_policy,
                     uint32_t index)
    : storage_(std::move(storage)),
      path_(path.MakeChildPath(index)),
      bson_value_(bson_value),
      duplicate_fields_policy_(duplicate_fields_policy) {
  UpdateStringPointers(bson_value_, std::get_if<std::string>(&storage_));
}

ValueImpl::ValueImpl(EmplaceEnabler, Storage storage, const Path& path,
                     const bson_value_t& bson_value,
                     Value::DuplicateFieldsPolicy duplicate_fields_policy,
                     const std::string& key)
    : storage_(std::move(storage)),
      path_(path.MakeChildPath(key)),
      bson_value_(bson_value),
      duplicate_fields_policy_(duplicate_fields_policy) {
  UpdateStringPointers(bson_value_, std::get_if<std::string>(&storage_));
}

ValueImpl::ValueImpl(const ValueImpl& other)
    : storage_(other.storage_),
      bson_value_(other.bson_value_),
      duplicate_fields_policy_(other.duplicate_fields_policy_) {
  const auto* parsed_ptr = other.parsed_value_.load();
  if (parsed_ptr) {
    auto deep_copy = std::visit(DeepCopyVisitor{}, *parsed_ptr);
    RelaxedSetParsedValue(parsed_value_, std::move(deep_copy));
  }

  UpdateStringPointers(bson_value_, std::get_if<std::string>(&storage_));
}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
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
  UpdateStringPointers(bson_value_, std::get_if<std::string>(&storage_));

  delete parsed_value_.load();
  parsed_value_ = rhs.parsed_value_.exchange(nullptr);

  duplicate_fields_policy_ = rhs.duplicate_fields_policy_;
  return *this;
}

bool ValueImpl::IsStorageOwner() const {
  class Visitor {
   public:
    Visitor(const ValueImpl& value) : value_(value) {}

    bool operator()(std::nullptr_t) const { return true; }

    bool operator()(const BsonHolder& bson) const {
      const bson_t* native_bson_ptr = bson.get();
      return value_.IsDocument() && value_.bson_value_.value.v_doc.data ==
                                        bson_get_data(native_bson_ptr);
    }

    bool operator()(const std::string&) const { return true; }

   private:
    const ValueImpl& value_;
  };

  CheckNotMissing();
  return std::visit(Visitor(*this), storage_);
}

bson_type_t ValueImpl::Type() const { return bson_value_.value_type; }

void ValueImpl::SetDuplicateFieldsPolicy(Value::DuplicateFieldsPolicy policy) {
  if (duplicate_fields_policy_ != policy) {
    delete parsed_value_.exchange(nullptr);
    duplicate_fields_policy_ = policy;
  }
}

ValueImplPtr ValueImpl::operator[](const std::string& name) {
  if (!IsMissing() && !IsNull()) {
    CheckIsDocument();
    EnsureParsed();
    const auto& parsed_doc = std::get<ParsedDocument>(*parsed_value_.load());
    auto it = parsed_doc.find(name);
    if (it != parsed_doc.end()) return it->second;
  }
  return std::make_shared<ValueImpl>(EmplaceEnabler{}, nullptr, path_,
                                     kDefaultBsonValue,
                                     duplicate_fields_policy_, name);
}

ValueImplPtr ValueImpl::operator[](uint32_t index) {
  if (IsNull()) {
    throw OutOfBoundsException(index, 0, path_.ToStringView());
  }

  CheckIsArray();
  EnsureParsed();
  CheckInBounds(index);
  return std::get<ParsedArray>(*parsed_value_.load())[index];
}

bool ValueImpl::HasMember(const std::string& name) {
  if (IsMissing() || IsNull()) return false;

  CheckIsDocument();
  EnsureParsed();
  return std::get<ParsedDocument>(*parsed_value_.load()).count(name);
}

ValueImplPtr ValueImpl::GetOrInsert(const std::string& key) {
  if (IsMissing() || IsNull()) {
    RelaxedSetParsedValue(parsed_value_, ParsedDocument());
    bson_value_.value_type = BSON_TYPE_DOCUMENT;
  }
  CheckIsDocument();
  EnsureParsed();
  return std::get<ParsedDocument>(*parsed_value_.load())
      .emplace(key, std::make_shared<ValueImpl>(EmplaceEnabler{}, nullptr,
                                                path_, kDefaultBsonValue,
                                                duplicate_fields_policy_, key))
      .first->second;
}

void ValueImpl::Resize(uint32_t new_size) {
  if (IsMissing() || IsNull()) {
    RelaxedSetParsedValue(parsed_value_, ParsedArray());
    bson_value_.value_type = BSON_TYPE_ARRAY;
  }
  CheckIsArray();
  EnsureParsed();
  auto& parsed_array = std::get<ParsedArray>(*parsed_value_.load());
  const auto old_size = parsed_array.size();
  parsed_array.resize(new_size);
  for (auto size = old_size; size < new_size; ++size) {
    parsed_array[size] = std::make_shared<ValueImpl>(
        EmplaceEnabler{}, nullptr, path_, kDefaultBsonValue,
        duplicate_fields_policy_, size);
  }
}

void ValueImpl::PushBack(ValueImplPtr impl) {
  if (IsMissing() || IsNull()) Resize(0);
  EnsureParsed();
  std::get<ParsedArray>(*parsed_value_.load()).push_back(std::move(impl));
}

void ValueImpl::Remove(const std::string& key) {
  CheckIsDocument();
  EnsureParsed();
  std::get<ParsedDocument>(*parsed_value_.load()).erase(key);
}

bool ValueImpl::IsEmpty() const {
  class Visitor {
   public:
    bool operator()(const ParsedArray& arr) const { return arr.empty(); }
    bool operator()(const ParsedDocument& doc) const { return doc.empty(); }
  };

  if (IsNull()) return true;
  CheckIsDocumentOrArray();

  auto* parsed_ptr = parsed_value_.load();
  if (!parsed_ptr) {
    constexpr uint32_t kEmptyDocSize = 5;
    return bson_value_.value.v_doc.data_len == kEmptyDocSize;
  }

  // do not parse here
  return std::visit(Visitor(), *parsed_ptr);
}

uint32_t ValueImpl::GetSize() const {
  class Visitor {
   public:
    uint32_t operator()(const ParsedDocument& doc) const { return doc.size(); }
    uint32_t operator()(const ParsedArray& array) const { return array.size(); }
  };

  if (IsNull()) return 0;
  CheckIsDocumentOrArray();

  auto* parsed_ptr = parsed_value_.load();
  if (!parsed_ptr) {
    uint32_t size = 0;
    ForEachValue(bson_value_.value.v_doc.data, bson_value_.value.v_doc.data_len,
                 path_, [&size](bson_iter_t*) { ++size; });
    return size;
  }

  return std::visit(Visitor(), *parsed_ptr);
}

std::string ValueImpl::GetPath() const { return path_.ToString(); }

ValueImpl::Iterator ValueImpl::Begin() {
  class Visitor {
   public:
    ValueImpl::Iterator operator()(const ParsedArray& arr) const {
      return arr.cbegin();
    }
    ValueImpl::Iterator operator()(const ParsedDocument& doc) const {
      return doc.cbegin();
    }
  };
  if (IsNull()) return {};
  CheckIsDocumentOrArray();
  EnsureParsed();
  return std::visit(Visitor{}, *parsed_value_.load());
}

ValueImpl::Iterator ValueImpl::End() {
  class Visitor {
   public:
    ValueImpl::Iterator operator()(const ParsedArray& arr) const {
      return arr.cend();
    }
    ValueImpl::Iterator operator()(const ParsedDocument& doc) const {
      return doc.cend();
    }
  };
  if (IsNull()) return {};
  CheckIsDocumentOrArray();
  EnsureParsed();
  return std::visit(Visitor{}, *parsed_value_.load());
}

ValueImpl::Iterator ValueImpl::Rbegin() {
  class Visitor {
   public:
    Visitor(const Path& path) : path_(path) {}

    ValueImpl::Iterator operator()(const ParsedArray& arr) const {
      return arr.crbegin();
    }
    ValueImpl::Iterator operator()(const ParsedDocument&) const {
      throw TypeMismatchException(BSON_TYPE_DOCUMENT, BSON_TYPE_ARRAY,
                                  path_.ToStringView());
    }

   private:
    const Path& path_;
  };
  if (IsNull()) return {};
  CheckNotMissing();
  EnsureParsed();
  return std::visit(Visitor{path_}, *parsed_value_.load());
}

ValueImpl::Iterator ValueImpl::Rend() {
  class Visitor {
   public:
    Visitor(const Path& path) : path_(path) {}

    ValueImpl::Iterator operator()(const ParsedArray& arr) const {
      return arr.crend();
    }
    ValueImpl::Iterator operator()(const ParsedDocument&) const {
      throw TypeMismatchException(BSON_TYPE_DOCUMENT, BSON_TYPE_ARRAY,
                                  path_.ToStringView());
    }

   private:
    const Path& path_;
  };
  if (IsNull()) return {};
  CheckNotMissing();
  EnsureParsed();
  return std::visit(Visitor{path_}, *parsed_value_.load());
}

bool ValueImpl::IsMissing() const { return Type() == BSON_TYPE_EOD; }
bool ValueImpl::IsNull() const { return Type() == BSON_TYPE_NULL; }
bool ValueImpl::IsArray() const { return Type() == BSON_TYPE_ARRAY; }
bool ValueImpl::IsDocument() const { return Type() == BSON_TYPE_DOCUMENT; }
bool ValueImpl::IsInt() const { return Type() == BSON_TYPE_INT32; }
bool ValueImpl::IsInt64() const { return Type() == BSON_TYPE_INT64 || IsInt(); }
bool ValueImpl::IsDouble() const {
  return Type() == BSON_TYPE_DOUBLE || IsInt64();
}
bool ValueImpl::IsBool() const { return Type() == BSON_TYPE_BOOL; }
bool ValueImpl::IsString() const { return Type() == BSON_TYPE_UTF8; }

void ValueImpl::EnsureParsed() {
  CheckNotMissing();
  if (parsed_value_.load() != nullptr) return;

  switch (bson_value_.value_type) {
    case BSON_TYPE_EOD:
      UASSERT(false);
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
    case BSON_TYPE_TIMESTAMP:
    case BSON_TYPE_INT64:
    case BSON_TYPE_DECIMAL128:
    case BSON_TYPE_MAXKEY:
    case BSON_TYPE_MINKEY:
      // break;

    // those types are not supported, but can be transferred between values
    case BSON_TYPE_UNDEFINED:
    case BSON_TYPE_REGEX:
    case BSON_TYPE_DBPOINTER:
    case BSON_TYPE_CODE:
    case BSON_TYPE_SYMBOL:
    case BSON_TYPE_CODEWSCOPE:
      break;

    case BSON_TYPE_ARRAY: {
      auto data =
          std::make_unique<ParsedValue>(std::in_place_type<ParsedArray>);
      auto& parsed_array = std::get<ParsedArray>(*data);

      ForEachValue(bson_value_.value.v_doc.data,
                   bson_value_.value.v_doc.data_len, path_,
                   [this, &parsed_array,
                    indexer = ArrayIndexer()](bson_iter_t* it) mutable {
                     std::string_view expected_key = indexer.GetKey();
                     std::string_view actual_key(bson_iter_key(it),
                                                 bson_iter_key_len(it));
                     if (expected_key != actual_key) {
                       throw ParseException(fmt::format(
                           "malformed BSON array at {}: index "
                           "mismatch, expected={}, got={}",
                           path_.ToStringView(), expected_key, actual_key));
                     }

                     const bson_value_t* iter_value = bson_iter_value(it);
                     if (!iter_value) {
                       throw ParseException(
                           fmt::format("malformed BSON element at {}[{}]",
                                       path_.ToStringView(), expected_key));
                     }
                     parsed_array.push_back(std::make_shared<ValueImpl>(
                         EmplaceEnabler{}, storage_, path_, *iter_value,
                         duplicate_fields_policy_, indexer.Index()));
                     indexer.Advance();
                   });
      AtomicSetParsedValue(parsed_value_, std::move(data));
      break;
    }
    case BSON_TYPE_DOCUMENT: {
      auto data =
          std::make_unique<ParsedValue>(std::in_place_type<ParsedDocument>);
      auto& parsed_doc = std::get<ParsedDocument>(*data);
      ForEachValue(
          bson_value_.value.v_doc.data, bson_value_.value.v_doc.data_len, path_,
          [this, &parsed_doc](bson_iter_t* it) {
            std::string_view key(bson_iter_key(it), bson_iter_key_len(it));
            const bson_value_t* iter_value = bson_iter_value(it);
            if (!iter_value) {
              throw ParseException(
                  fmt::format("malformed BSON element at {}.{}",
                              path_.ToStringView(), key));
            }
            auto [parsed_it, is_new] = parsed_doc.emplace(
                std::string(key),
                std::make_shared<ValueImpl>(
                    EmplaceEnabler{}, storage_, path_, *iter_value,
                    duplicate_fields_policy_, std::string(key)));
            if (!is_new) {
              switch (duplicate_fields_policy_) {
                case Value::DuplicateFieldsPolicy::kForbid:
                  throw ParseException(fmt::format("duplicate key '{}' at {}",
                                                   key, path_.ToStringView()));
                case Value::DuplicateFieldsPolicy::kUseFirst:
                  // leave current value as is
                  break;
                case Value::DuplicateFieldsPolicy::kUseLast:
                  // replace it
                  parsed_it->second = std::make_shared<ValueImpl>(
                      EmplaceEnabler{}, storage_, path_, *iter_value,
                      duplicate_fields_policy_, std::string(key));
              }
            }
          });
      AtomicSetParsedValue(parsed_value_, std::move(data));
      break;
    }
  }
}

void ValueImpl::SyncBsonValue() {
  // either primitive type or was never touched
  if (parsed_value_.load() == nullptr) return;

  UASSERT(IsDocument() || IsArray());

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

  class Visitor {
   public:
    explicit Visitor(const ValueImpl& rhs) : rhs_(*rhs.parsed_value_.load()) {}

    bool operator()(const ParsedDocument& lhs_doc) const {
      const auto& rhs_doc = std::get<ParsedDocument>(rhs_);
      if (lhs_doc.size() != rhs_doc.size()) return false;
      for (const auto& [k, v] : lhs_doc) {
        auto rhs_it = rhs_doc.find(k);
        if (rhs_it == rhs_doc.end() || *rhs_it->second != *v) return false;
      }
      return true;
    }

    bool operator()(const ParsedArray& lhs_arr) const {
      const auto& rhs_arr = std::get<ParsedArray>(rhs_);
      if (lhs_arr.size() != rhs_arr.size()) return false;
      for (size_t i = 0; i < lhs_arr.size(); ++i) {
        if (*lhs_arr[i] != *rhs_arr[i]) return false;
      }
      return true;
    }

   private:
    const ParsedValue& rhs_;
  };
  const auto* parsed_ptr = parsed_value_.load();
  if (!parsed_ptr) {
    UASSERT(!rhs.parsed_value_.load());
    return bson_value_ == rhs.bson_value_;
  }

  UASSERT(parsed_ptr->index() == rhs.parsed_value_.load()->index());
  return std::visit(Visitor(rhs), *parsed_ptr);
}

bool ValueImpl::operator!=(ValueImpl& rhs) { return !(*this == rhs); }

void ValueImpl::CheckNotMissing() const {
  if (Type() == BSON_TYPE_EOD) {
    throw MemberMissingException(path_.ToStringView());
  }
}

void ValueImpl::CheckIsDocumentOrArray() const {
  CheckNotMissing();
  if (!IsDocument() && !IsArray()) {
    throw TypeMismatchException(bson_value_.value_type, BSON_TYPE_DOCUMENT,
                                path_.ToStringView());
  }
}

void ValueImpl::CheckIsDocument() const {
  CheckNotMissing();
  if (!IsDocument()) {
    throw TypeMismatchException(bson_value_.value_type, BSON_TYPE_DOCUMENT,
                                path_.ToStringView());
  }
}

void ValueImpl::CheckIsArray() const {
  CheckNotMissing();
  if (!IsArray()) {
    throw TypeMismatchException(bson_value_.value_type, BSON_TYPE_ARRAY,
                                path_.ToStringView());
  }
}

void ValueImpl::CheckInBounds(uint32_t idx) const {
  CheckIsArray();
  const auto size = GetSize();
  if (idx >= size) {
    throw OutOfBoundsException(idx, size, path_.ToStringView());
  }
}

}  // namespace formats::bson::impl

USERVER_NAMESPACE_END
