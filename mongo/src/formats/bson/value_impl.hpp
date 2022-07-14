#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <variant>

#include <bson/bson.h>

#include <userver/formats/bson/types.hpp>
#include <userver/formats/bson/value.hpp>
#include <userver/formats/common/path.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson::impl {

class BsonBuilder;

class ValueImpl {
 public:
  enum class DocumentKind { kDocument, kArray };

  using Iterator = std::variant<ParsedArray::const_iterator,
                                ParsedArray::const_reverse_iterator,
                                ParsedDocument::const_iterator>;

  ValueImpl();
  ~ValueImpl();
  explicit ValueImpl(BsonHolder, DocumentKind = DocumentKind::kDocument);

  explicit ValueImpl(std::nullptr_t);
  explicit ValueImpl(bool);
  explicit ValueImpl(int32_t);
  explicit ValueImpl(int64_t);
  explicit ValueImpl(double);
  explicit ValueImpl(const char*);
  explicit ValueImpl(std::string);
  explicit ValueImpl(const std::chrono::system_clock::time_point&);
  explicit ValueImpl(const Oid&);
  explicit ValueImpl(Binary);
  explicit ValueImpl(const Decimal128&);
  explicit ValueImpl(MinKey);
  explicit ValueImpl(MaxKey);
  explicit ValueImpl(const Timestamp&);

  // Copy/move preserves paths
  ValueImpl(const ValueImpl&);
  ValueImpl(ValueImpl&&) noexcept;
  ValueImpl& operator=(const ValueImpl&);
  ValueImpl& operator=(ValueImpl&&) noexcept;

  bool IsStorageOwner() const;
  bson_type_t Type() const;

  void SetDuplicateFieldsPolicy(Value::DuplicateFieldsPolicy);

  ValueImplPtr operator[](const std::string& name);
  ValueImplPtr operator[](uint32_t index);

  bool HasMember(const std::string& name);

  ValueImplPtr GetOrInsert(const std::string& key);
  void Resize(uint32_t size);
  void PushBack(ValueImplPtr);
  void Remove(const std::string& key);

  bool IsEmpty() const;
  uint32_t GetSize() const;
  std::string GetPath() const;

  Iterator Begin();
  Iterator End();
  Iterator Rbegin();
  Iterator Rend();

  bool IsMissing() const;
  bool IsNull() const;
  bool IsDocument() const;
  bool IsArray() const;
  bool IsBool() const;
  bool IsInt() const;
  bool IsInt64() const;
  bool IsDouble() const;
  bool IsString() const;

  bool operator==(ValueImpl&);
  bool operator!=(ValueImpl&);

  void EnsureParsed();
  void SyncBsonValue();

  const bson_value_t* GetNative() const { return &bson_value_; }
  const BsonHolder& GetBson() const { return std::get<BsonHolder>(storage_); }

  void CheckNotMissing() const;
  void CheckIsDocumentOrArray() const;
  void CheckIsDocument() const;
  void CheckIsArray() const;
  void CheckInBounds(uint32_t idx) const;

 private:
  class EmplaceEnabler;

 public:
  using Storage = std::variant<std::nullptr_t, BsonHolder, std::string>;
  using ParsedValue = std::variant<ParsedDocument, ParsedArray>;

  ValueImpl(EmplaceEnabler, Storage, const Path&, const bson_value_t&,
            Value::DuplicateFieldsPolicy, uint32_t);
  ValueImpl(EmplaceEnabler, Storage, const Path&, const bson_value_t&,
            Value::DuplicateFieldsPolicy, const std::string&);

 private:
  friend class BsonBuilder;

  Storage storage_;
  Path path_;
  bson_value_t bson_value_;
  std::atomic<ParsedValue*> parsed_value_{nullptr};
  Value::DuplicateFieldsPolicy duplicate_fields_policy_{
      Value::DuplicateFieldsPolicy::kForbid};
};

}  // namespace formats::bson::impl

USERVER_NAMESPACE_END
