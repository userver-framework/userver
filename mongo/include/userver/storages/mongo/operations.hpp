#pragma once

/// @file userver/storages/mongo/operations.hpp
/// @brief Collection operation models

#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/value.hpp>
#include <userver/storages/mongo/options.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {
class CDriverCollectionImpl;
}  // namespace storages::mongo::impl::cdriver

/// Collection operations
namespace storages::mongo::operations {

/// Counts documents matching the filter
class Count {
 public:
  explicit Count(formats::bson::Document filter);
  ~Count();

  Count(const Count&);
  Count(Count&&) noexcept;
  Count& operator=(const Count&);
  Count& operator=(Count&&) noexcept;

  void SetOption(const options::ReadPreference&);
  void SetOption(options::ReadPreference::Mode);
  void SetOption(options::ReadConcern);
  void SetOption(options::Skip);
  void SetOption(options::Limit);
  void SetOption(options::ForceCountImpl);
  void SetOption(const options::MaxServerTime&);

 private:
  friend class storages::mongo::impl::cdriver::CDriverCollectionImpl;

  class Impl;
  static constexpr size_t kSize = 96;
  static constexpr size_t kAlignment = 8;
  // MAC_COMPAT: std::string size differs
  utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

/// Returns approximate number of documents in a collection
class CountApprox {
 public:
  CountApprox();
  ~CountApprox();

  CountApprox(const CountApprox&);
  CountApprox(CountApprox&&) noexcept;
  CountApprox& operator=(const CountApprox&);
  CountApprox& operator=(CountApprox&&) noexcept;

  void SetOption(const options::ReadPreference&);
  void SetOption(options::ReadPreference::Mode);
  void SetOption(options::ReadConcern);
  void SetOption(options::Skip);
  void SetOption(options::Limit);
  void SetOption(const options::MaxServerTime&);

 private:
  friend class storages::mongo::impl::cdriver::CDriverCollectionImpl;

  class Impl;
  static constexpr size_t kSize = 72;
  static constexpr size_t kAlignment = 8;
  // MAC_COMPAT: std::string size differs
  utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

/// Retrieves documents matching the filter
class Find {
 public:
  explicit Find(formats::bson::Document filter);
  ~Find();

  Find(const Find&);
  Find(Find&&) noexcept;
  Find& operator=(const Find&);
  Find& operator=(Find&&) noexcept;

  void SetOption(const options::ReadPreference&);
  void SetOption(options::ReadPreference::Mode);
  void SetOption(options::ReadConcern);
  void SetOption(options::Skip);
  void SetOption(options::Limit);
  void SetOption(options::Projection);
  void SetOption(const options::Sort&);
  void SetOption(const options::Hint&);
  void SetOption(options::AllowPartialResults);
  void SetOption(options::Tailable);
  void SetOption(const options::Comment&);
  void SetOption(const options::MaxServerTime&);

 private:
  friend class storages::mongo::impl::cdriver::CDriverCollectionImpl;

  class Impl;
  static constexpr size_t kSize = 96;
  static constexpr size_t kAlignment = 8;
  // MAC_COMPAT: std::string size differs
  utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

/// Inserts a single document
class InsertOne {
 public:
  explicit InsertOne(formats::bson::Document document);
  ~InsertOne();

  InsertOne(const InsertOne&);
  InsertOne(InsertOne&&) noexcept;
  InsertOne& operator=(const InsertOne&);
  InsertOne& operator=(InsertOne&&) noexcept;

  void SetOption(options::WriteConcern::Level);
  void SetOption(const options::WriteConcern&);
  void SetOption(options::SuppressServerExceptions);

 private:
  friend class storages::mongo::impl::cdriver::CDriverCollectionImpl;

  class Impl;
  static constexpr size_t kSize = 80;
  static constexpr size_t kAlignment = 8;
  // MAC_COMPAT: std::string size differs
  utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

/// Inserts multiple documents
class InsertMany {
 public:
  InsertMany();
  explicit InsertMany(std::vector<formats::bson::Document> documents);
  ~InsertMany();

  InsertMany(const InsertMany&);
  InsertMany(InsertMany&&) noexcept;
  InsertMany& operator=(const InsertMany&);
  InsertMany& operator=(InsertMany&&) noexcept;

  void Append(formats::bson::Document document);

  void SetOption(options::Unordered);
  void SetOption(options::WriteConcern::Level);
  void SetOption(const options::WriteConcern&);
  void SetOption(options::SuppressServerExceptions);

 private:
  friend class storages::mongo::impl::cdriver::CDriverCollectionImpl;

  class Impl;
  static constexpr size_t kSize = 88;
  static constexpr size_t kAlignment = 8;
  // MAC_COMPAT: std::string size differs
  utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

/// Replaces a single document
class ReplaceOne {
 public:
  ReplaceOne(formats::bson::Document selector,
             formats::bson::Document replacement);
  ~ReplaceOne();

  ReplaceOne(const ReplaceOne&);
  ReplaceOne(ReplaceOne&&) noexcept;
  ReplaceOne& operator=(const ReplaceOne&);
  ReplaceOne& operator=(ReplaceOne&&) noexcept;

  void SetOption(options::Upsert);
  void SetOption(options::WriteConcern::Level);
  void SetOption(const options::WriteConcern&);
  void SetOption(options::SuppressServerExceptions);

 private:
  friend class storages::mongo::impl::cdriver::CDriverCollectionImpl;

  class Impl;
  static constexpr size_t kSize = 96;
  static constexpr size_t kAlignment = 8;
  // MAC_COMPAT: std::string size differs
  utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

/// Updates documents
class Update {
 public:
  enum class Mode { kSingle, kMulti };

  Update(Mode mode, formats::bson::Document selector,
         formats::bson::Document update);
  ~Update();

  Update(const Update&);
  Update(Update&&) noexcept;
  Update& operator=(const Update&);
  Update& operator=(Update&&) noexcept;

  void SetOption(options::Upsert);
  void SetOption(options::RetryDuplicateKey);
  void SetOption(options::WriteConcern::Level);
  void SetOption(const options::WriteConcern&);
  void SetOption(options::SuppressServerExceptions);

 private:
  friend class storages::mongo::impl::cdriver::CDriverCollectionImpl;

  class Impl;
  static constexpr size_t kSize = 96;
  static constexpr size_t kAlignment = 8;
  // MAC_COMPAT: std::string size differs
  utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

/// Deletes documents
class Delete {
 public:
  enum class Mode { kSingle, kMulti };

  Delete(Mode mode, formats::bson::Document selector);
  ~Delete();

  Delete(const Delete&);
  Delete(Delete&&) noexcept;
  Delete& operator=(const Delete&);
  Delete& operator=(Delete&&) noexcept;

  void SetOption(options::WriteConcern::Level);
  void SetOption(const options::WriteConcern&);
  void SetOption(options::SuppressServerExceptions);

 private:
  friend class storages::mongo::impl::cdriver::CDriverCollectionImpl;

  class Impl;
  static constexpr size_t kSize = 80;
  static constexpr size_t kAlignment = 8;
  // MAC_COMPAT: std::string size differs
  utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

/// Atomically updates a document and returns either previous or new version
class FindAndModify {
 public:
  FindAndModify(formats::bson::Document query,
                const formats::bson::Document& update);
  ~FindAndModify();

  FindAndModify(const FindAndModify&) = delete;
  FindAndModify(FindAndModify&&) noexcept;
  FindAndModify& operator=(const FindAndModify&) = delete;
  FindAndModify& operator=(FindAndModify&&) noexcept;

  void SetOption(options::ReturnNew);
  void SetOption(options::Upsert);
  void SetOption(options::RetryDuplicateKey);
  void SetOption(const options::Sort&);
  void SetOption(options::Projection);
  void SetOption(options::WriteConcern::Level);
  void SetOption(const options::WriteConcern&);
  void SetOption(const options::MaxServerTime&);

 private:
  friend class storages::mongo::impl::cdriver::CDriverCollectionImpl;

  class Impl;
  static constexpr size_t kSize = 80;
  static constexpr size_t kAlignment = 8;
  // MAC_COMPAT: std::string size differs
  utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

/// Atomically removes a document and returns it
class FindAndRemove {
 public:
  explicit FindAndRemove(formats::bson::Document query);
  ~FindAndRemove();

  FindAndRemove(const FindAndRemove&) = delete;
  FindAndRemove(FindAndRemove&&) noexcept;
  FindAndRemove& operator=(const FindAndRemove&) = delete;
  FindAndRemove& operator=(FindAndRemove&&) noexcept;

  void SetOption(const options::Sort&);
  void SetOption(options::Projection);
  void SetOption(options::WriteConcern::Level);
  void SetOption(const options::WriteConcern&);
  void SetOption(const options::MaxServerTime&);

 private:
  friend class storages::mongo::impl::cdriver::CDriverCollectionImpl;

  class Impl;
  static constexpr size_t kSize = 72;
  static constexpr size_t kAlignment = 8;
  // MAC_COMPAT: std::string size differs
  utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

/// Runs an aggregation pipeline
class Aggregate {
 public:
  explicit Aggregate(formats::bson::Value pipeline);
  ~Aggregate();

  Aggregate(const Aggregate&);
  Aggregate(Aggregate&&) noexcept;
  Aggregate& operator=(const Aggregate&);
  Aggregate& operator=(Aggregate&&) noexcept;

  void SetOption(const options::ReadPreference&);
  void SetOption(options::ReadPreference::Mode);
  void SetOption(options::ReadConcern);
  void SetOption(const options::WriteConcern&);
  void SetOption(options::WriteConcern::Level);
  void SetOption(const options::Hint&);
  void SetOption(const options::Comment&);
  void SetOption(const options::MaxServerTime&);

 private:
  friend class storages::mongo::impl::cdriver::CDriverCollectionImpl;

  class Impl;
  static constexpr size_t kSize = 120;
  static constexpr size_t kAlignment = 8;
  // MAC_COMPAT: std::string size differs
  utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

class Drop {
 public:
  Drop();
  ~Drop();

  Drop(const Drop&);
  Drop(Drop&&) noexcept;
  Drop& operator=(const Drop&);
  Drop& operator=(Drop&&) noexcept;

  void SetOption(const options::WriteConcern&);
  void SetOption(options::WriteConcern::Level);

 private:
  friend class storages::mongo::impl::cdriver::CDriverCollectionImpl;

  class Impl;
  static constexpr size_t kSize = 56;
  static constexpr size_t kAlignment = 8;
  // MAC_COMPAT: std::string size differs
  utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

}  // namespace storages::mongo::operations

USERVER_NAMESPACE_END
