#pragma once

/// @file storages/mongo/bulk.hpp
/// @brief Bulk collection operation model

#include <storages/mongo/bulk_ops.hpp>
#include <storages/mongo/options.hpp>
#include <utils/fast_pimpl.hpp>

namespace storages::mongo {
class Collection;
}  // namespace storages::mongo

namespace storages::mongo::operations {

/// Efficiently executes a number of operations over a single collection
class Bulk {
 public:
  enum class Mode { kOrdered, kUnordered };

  explicit Bulk(Mode);
  ~Bulk();

  Bulk(const Bulk&) = delete;
  Bulk(Bulk&&) noexcept;
  Bulk& operator=(const Bulk&) = delete;
  Bulk& operator=(Bulk&&) noexcept;

  bool IsEmpty() const;

  void SetOption(options::WriteConcern::Level);
  void SetOption(const options::WriteConcern&);
  void SetOption(options::SuppressServerExceptions);

  /// Inserts a single document
  template <typename... Options>
  void InsertOne(formats::bson::Document document, Options&&... options);

  /// @brief Replaces a single matching document
  /// @see options::Upsert
  template <typename... Options>
  void ReplaceOne(formats::bson::Document selector,
                  formats::bson::Document replacement, Options&&... options);

  /// @brief Updates a single matching document
  /// @see options::Upsert
  template <typename... Options>
  void UpdateOne(formats::bson::Document selector,
                 formats::bson::Document update, Options&&... options);

  /// @brief Updates all matching documents
  /// @see options::Upsert
  template <typename... Options>
  void UpdateMany(formats::bson::Document selector,
                  formats::bson::Document update, Options&&... options);

  /// Deletes a single matching document
  template <typename... Options>
  void DeleteOne(formats::bson::Document selector, Options&&... options);

  /// Deletes all matching documents
  template <typename... Options>
  void DeleteMany(formats::bson::Document selector, Options&&... options);

  /// @name Prepared sub-operation inserters
  /// @{
  void Append(const bulk_ops::InsertOne&);
  void Append(const bulk_ops::ReplaceOne&);
  void Append(const bulk_ops::Update&);
  void Append(const bulk_ops::Delete&);
  /// @}

 private:
  friend class ::storages::mongo::Collection;

  class Impl;
  static constexpr size_t kSize = 16;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment, true> impl_;
};

template <typename... Options>
void Bulk::InsertOne(formats::bson::Document document, Options&&... options) {
  bulk_ops::InsertOne insert_subop(std::move(document));
  (insert_subop.SetOption(std::forward<Options>(options)), ...);
  Append(insert_subop);
}

template <typename... Options>
void Bulk::ReplaceOne(formats::bson::Document selector,
                      formats::bson::Document replacement,
                      Options&&... options) {
  bulk_ops::ReplaceOne replace_subop(std::move(selector),
                                     std::move(replacement));
  (replace_subop.SetOption(std::forward<Options>(options)), ...);
  Append(replace_subop);
}

template <typename... Options>
void Bulk::UpdateOne(formats::bson::Document selector,
                     formats::bson::Document update, Options&&... options) {
  bulk_ops::Update update_subop(bulk_ops::Update::Mode::kSingle,
                                std::move(selector), std::move(update));
  (update_subop.SetOption(std::forward<Options>(options)), ...);
  Append(update_subop);
}

template <typename... Options>
void Bulk::UpdateMany(formats::bson::Document selector,
                      formats::bson::Document update, Options&&... options) {
  bulk_ops::Update update_subop(bulk_ops::Update::Mode::kMulti,
                                std::move(selector), std::move(update));
  (update_subop.SetOption(std::forward<Options>(options)), ...);
  Append(update_subop);
}

template <typename... Options>
void Bulk::DeleteOne(formats::bson::Document selector, Options&&... options) {
  bulk_ops::Delete delete_subop(bulk_ops::Delete::Mode::kSingle,
                                std::move(selector));
  (delete_subop.SetOption(std::forward<Options>(options)), ...);
  Append(delete_subop);
}

template <typename... Options>
void Bulk::DeleteMany(formats::bson::Document selector, Options&&... options) {
  bulk_ops::Delete delete_subop(bulk_ops::Delete::Mode::kMulti,
                                std::move(selector));
  (delete_subop.SetOption(std::forward<Options>(options)), ...);
  Append(delete_subop);
}

}  // namespace storages::mongo::operations
