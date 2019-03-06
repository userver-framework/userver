#pragma once

/// @file storages/mongo_ng/operations.hpp
/// @brief Collection operation models

#include <formats/bson/document.hpp>
#include <storages/mongo_ng/options.hpp>
#include <utils/fast_pimpl.hpp>

namespace storages::mongo_ng {
class Collection;
}  // namespace storages::mongo_ng

namespace storages::mongo_ng::operations {

/// Counts documents matching the filter
class Count {
 public:
  explicit Count(formats::bson::Document filter);
  ~Count();

  void SetOption(const options::ReadPreference&);
  void SetOption(options::ReadPreference::Mode);
  void SetOption(options::ReadConcern);
  void SetOption(options::Skip);
  void SetOption(options::Limit);

 private:
  friend class ::storages::mongo_ng::Collection;

  class Impl;
  static constexpr size_t kSize = 40;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment, true> impl_;
};

/// Returns approximate number of documents in a collection
class CountApprox {
 public:
  CountApprox();
  ~CountApprox();

  void SetOption(const options::ReadPreference&);
  void SetOption(options::ReadPreference::Mode);
  void SetOption(options::ReadConcern);
  void SetOption(options::Skip);
  void SetOption(options::Limit);

 private:
  friend class ::storages::mongo_ng::Collection;

  class Impl;
  static constexpr size_t kSize = 24;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment, true> impl_;
};

/// Retrieves documents matching the filter
class Find {
 public:
  explicit Find(formats::bson::Document filter);
  ~Find();

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
  void SetOption(options::BatchSize);
  void SetOption(const options::MaxServerTime&);

 private:
  friend class ::storages::mongo_ng::Collection;

  class Impl;
  static constexpr size_t kSize = 40;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment, true> impl_;
};

/// Inserts a single document
class InsertOne {
 public:
  explicit InsertOne(formats::bson::Document document);
  ~InsertOne();

  void SetOption(options::WriteConcern::Level);
  void SetOption(const options::WriteConcern&);
  void SetOption(options::SuppressServerExceptions);

 private:
  friend class ::storages::mongo_ng::Collection;

  class Impl;
  static constexpr size_t kSize = 40;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment, true> impl_;
};

/// Inserts multiple documents
class InsertMany {
 public:
  InsertMany();
  explicit InsertMany(std::vector<formats::bson::Document> documents);
  ~InsertMany();

  void Append(formats::bson::Document document);

  void SetOption(options::Unordered);
  void SetOption(options::WriteConcern::Level);
  void SetOption(const options::WriteConcern&);
  void SetOption(options::SuppressServerExceptions);

 private:
  friend class ::storages::mongo_ng::Collection;

  class Impl;
  static constexpr size_t kSize = 48;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment, true> impl_;
};

/// Replaces a single document
class ReplaceOne {
 public:
  ReplaceOne(formats::bson::Document selector,
             formats::bson::Document replacement);
  ~ReplaceOne();

  void SetOption(options::Upsert);
  void SetOption(options::WriteConcern::Level);
  void SetOption(const options::WriteConcern&);
  void SetOption(options::SuppressServerExceptions);

 private:
  friend class ::storages::mongo_ng::Collection;

  class Impl;
  static constexpr size_t kSize = 56;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment, true> impl_;
};

/// Updates documents
class Update {
 public:
  enum class Mode { kSingle, kMulti };

  Update(Mode mode, formats::bson::Document selector,
         formats::bson::Document update);
  ~Update();

  void SetOption(options::Upsert);
  void SetOption(options::WriteConcern::Level);
  void SetOption(const options::WriteConcern&);
  void SetOption(options::SuppressServerExceptions);

 private:
  friend class ::storages::mongo_ng::Collection;

  class Impl;
  static constexpr size_t kSize = 56;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment, true> impl_;
};

/// Deletes documents
class Delete {
 public:
  enum class Mode { kSingle, kMulti };

  Delete(Mode mode, formats::bson::Document selector);
  ~Delete();

  void SetOption(options::WriteConcern::Level);
  void SetOption(const options::WriteConcern&);
  void SetOption(options::SuppressServerExceptions);

 private:
  friend class ::storages::mongo_ng::Collection;

  class Impl;
  static constexpr size_t kSize = 40;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment, true> impl_;
};

/// Atomically updates a document and returns either previous or new version
class FindAndModify {
 public:
  FindAndModify(formats::bson::Document query,
                const formats::bson::Document& update);
  ~FindAndModify();

  void SetOption(options::Upsert);
  void SetOption(options::ReturnNew);
  void SetOption(const options::Sort&);
  void SetOption(options::Projection);
  void SetOption(options::WriteConcern::Level);
  void SetOption(const options::WriteConcern&);
  void SetOption(const options::MaxServerTime&);

 private:
  friend class ::storages::mongo_ng::Collection;

  class Impl;
  static constexpr size_t kSize = 24;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment, true> impl_;
};

/// Atomically removes a document and returns it
class FindAndRemove {
 public:
  explicit FindAndRemove(formats::bson::Document query);
  ~FindAndRemove();

  void SetOption(const options::Sort&);
  void SetOption(options::Projection);
  void SetOption(options::WriteConcern::Level);
  void SetOption(const options::WriteConcern&);
  void SetOption(const options::MaxServerTime&);

 private:
  friend class ::storages::mongo_ng::Collection;

  class Impl;
  static constexpr size_t kSize = 24;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment, true> impl_;
};

}  // namespace storages::mongo_ng::operations
