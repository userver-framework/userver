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
struct Find {
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

}  // namespace storages::mongo_ng::operations
