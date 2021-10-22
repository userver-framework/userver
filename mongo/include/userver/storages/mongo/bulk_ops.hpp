#pragma once

#include <userver/formats/bson/document.hpp>
#include <userver/storages/mongo/options.hpp>
#include <userver/utils/fast_pimpl.hpp>

/// @file userver/storages/mongo/bulk_ops.hpp
/// @brief Bulk sub-operation models

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::operations {
class Bulk;
}  // namespace storages::mongo::operations

/// Bulk sub-operations
namespace storages::mongo::bulk_ops {

/// Inserts a single document as part of bulk operation
class InsertOne {
 public:
  explicit InsertOne(formats::bson::Document document);
  ~InsertOne();

  InsertOne(const InsertOne&);
  InsertOne(InsertOne&&) noexcept;
  InsertOne& operator=(const InsertOne&);
  InsertOne& operator=(InsertOne&&) noexcept;

  void SetOption() const {}

 private:
  friend class storages::mongo::operations::Bulk;

  class Impl;
  static constexpr size_t kSize = 16;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment, true> impl_;
};

/// Replaces a single document as part of bulk operation
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

 private:
  friend class storages::mongo::operations::Bulk;

  class Impl;
  static constexpr size_t kSize = 48;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment, true> impl_;
};

/// Updates documents as part of bulk operation
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

 private:
  friend class storages::mongo::operations::Bulk;

  class Impl;
  static constexpr size_t kSize = 56;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment, true> impl_;
};

/// Deletes documents as part of bulk operation
class Delete {
 public:
  enum class Mode { kSingle, kMulti };

  Delete(Mode mode, formats::bson::Document selector);
  ~Delete();

  Delete(const Delete&);
  Delete(Delete&&) noexcept;
  Delete& operator=(const Delete&);
  Delete& operator=(Delete&&) noexcept;

  void SetOption() const {}

 private:
  friend class storages::mongo::operations::Bulk;

  class Impl;
  static constexpr size_t kSize = 24;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment, true> impl_;
};

}  // namespace storages::mongo::bulk_ops

USERVER_NAMESPACE_END
