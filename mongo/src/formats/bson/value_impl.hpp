#pragma once

#include <chrono>
#include <memory>

#include <bson/bson.h>
#include <boost/variant.hpp>

#include <formats/bson/types.hpp>

namespace formats::bson::impl {

class BsonBuilder;

class ValueImpl {
 public:
  enum class DocumentKind { kDocument, kArray };

  using Iterator = boost::variant<ParsedDocument::const_iterator,
                                  ParsedArray::const_iterator>;

  ValueImpl();
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

  ValueImplPtr operator[](const std::string& name);
  ValueImplPtr operator[](uint32_t index);

  bool HasMember(const std::string& name);

  ValueImplPtr GetOrInsert(const std::string& key);
  void Resize(uint32_t size);
  void PushBack(ValueImplPtr);

  uint32_t GetSize() const;
  std::string GetPath() const;

  Iterator Begin();
  Iterator End();

  bool IsMissing() const;
  bool IsNull() const;
  bool IsDocument() const;
  bool IsArray() const;

  bool operator==(ValueImpl&);
  bool operator!=(ValueImpl&);

  void EnsureParsed();
  void SyncBsonValue();

  const bson_value_t* GetNative() const { return &bson_value_; }
  const BsonHolder& GetBson() const { return boost::get<BsonHolder>(storage_); }

  void CheckNotMissing() const;
  void CheckIsDocumentOrArray() const;
  void CheckIsDocument() const;
  void CheckIsArray() const;
  void CheckInBounds(uint32_t idx) const;

 private:
  class EmplaceEnabler;

 public:
  using Storage = boost::variant<std::nullptr_t, BsonHolder, std::string>;
  using ParsedValue =
      boost::variant<std::nullptr_t, ParsedDocument, ParsedArray>;

  ValueImpl(EmplaceEnabler, Storage, Path, const bson_value_t&, uint32_t);
  ValueImpl(EmplaceEnabler, Storage, Path, const bson_value_t&, std::string);

 private:
  friend class BsonBuilder;

  Storage storage_;
  Path path_;
  bson_value_t bson_value_;
  ParsedValue parsed_value_;
};

}  // namespace formats::bson::impl
