#include <storages/mongo/collection.hpp>

#include <mongoc/mongoc.h>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/third_party/mnmlstc/core/optional.hpp>
#include <mongocxx/exception/write_exception.hpp>

#include <engine/async.hpp>

#include "bulk_impl.hpp"
#include "collection_impl.hpp"
#include "cursor_impl.hpp"
#include "pool_impl.hpp"

namespace storages {
namespace mongo {

namespace {

template <typename T>
boost::optional<T> ToBoostOptional(core::optional<T>&& core_optional) {
  if (!core_optional) return {};
  return std::move(*core_optional);
}

bool IsDuplicateKeyError(const mongocxx::write_exception& ex) {
  return ex.code().value() == MONGOC_ERROR_DUPLICATE_KEY;
}

DocumentValue BuildFields(std::initializer_list<std::string> fields) {
  bsoncxx::builder::basic::document builder;
  for (const auto& field : fields) {
    builder.append(bsoncxx::builder::basic::kvp(field, 1));
  }
  return builder.extract();
}

}  // namespace

FieldsToReturn::FieldsToReturn(DocumentValue fields)
    : fields_(std::move(fields)) {}

FieldsToReturn::FieldsToReturn(std::initializer_list<std::string> fields)
    : fields_(BuildFields(fields)) {}

FieldsToReturn::operator bsoncxx::document::view_or_value() && {
  return std::move(fields_);
}

Collection::Collection(std::shared_ptr<impl::CollectionImpl>&& impl)
    : impl_(std::move(impl)) {}

engine::TaskWithResult<boost::optional<DocumentValue>> Collection::FindOne(
    DocumentValue query, mongocxx::options::find options) const {
  return engine::Async(
      impl_->GetPool().GetTaskProcessor(),
      [impl = impl_](auto&& query, const auto& options) {
        auto conn = impl->GetPool().Acquire();
        return ToBoostOptional(
            impl->GetCollection(conn).find_one(std::move(query), options));
      },
      std::move(query), std::move(options));
}

engine::TaskWithResult<boost::optional<DocumentValue>> Collection::FindOne(
    DocumentValue query, FieldsToReturn fields,
    mongocxx::options::find options) const {
  options.projection(std::move(fields));
  return FindOne(std::move(query), std::move(options));
}

engine::TaskWithResult<Cursor> Collection::Find(
    DocumentValue query, mongocxx::options::find options) const {
  return engine::Async(impl_->GetPool().GetTaskProcessor(),
                       [impl = impl_](auto&& query, const auto& options) {
                         auto conn = impl->GetPool().Acquire();
                         auto cursor = impl->GetCollection(conn).find(
                             std::move(query), options);
                         return Cursor(std::make_unique<impl::CursorImpl>(
                             impl->GetPool().GetTaskProcessor(),
                             std::move(conn), std::move(cursor)));
                       },
                       std::move(query), std::move(options));
}

engine::TaskWithResult<Cursor> Collection::Find(
    DocumentValue query, FieldsToReturn fields,
    mongocxx::options::find options) const {
  options.projection(std::move(fields));
  return Find(std::move(query), std::move(options));
}

engine::TaskWithResult<boost::optional<DocumentValue>>
Collection::FindOneAndDelete(DocumentValue query,
                             mongocxx::options::find_one_and_delete options) {
  return engine::Async(
      impl_->GetPool().GetTaskProcessor(),
      [impl = impl_](auto&& query, const auto& options) {
        auto conn = impl->GetPool().Acquire();
        return ToBoostOptional(impl->GetCollection(conn).find_one_and_delete(
            std::move(query), options));
      },
      std::move(query), std::move(options));
}

engine::TaskWithResult<boost::optional<DocumentValue>>
Collection::FindOneAndReplace(DocumentValue query, DocumentValue replacement,
                              mongocxx::options::find_one_and_replace options) {
  if (!options.return_document()) {
    options.return_document(mongocxx::options::return_document::k_after);
  }

  return engine::Async(
      impl_->GetPool().GetTaskProcessor(),
      [impl = impl_](auto&& query, auto&& replacement, const auto& options) {
        auto conn = impl->GetPool().Acquire();
        try {
          return ToBoostOptional(impl->GetCollection(conn).find_one_and_replace(
              query.view(), replacement.view(), options));
        } catch (const mongocxx::write_exception& ex) {
          // Retry duplicate key upsert once, could've been race
          if (IsDuplicateKeyError(ex) && options.upsert() &&
              *options.upsert()) {
            return ToBoostOptional(
                impl->GetCollection(conn).find_one_and_replace(
                    query.view(), replacement.view(), options));
          } else {
            throw;
          }
        }
      },
      std::move(query), std::move(replacement), std::move(options));
}

engine::TaskWithResult<boost::optional<DocumentValue>>
Collection::FindOneAndUpdate(DocumentValue query, DocumentValue update,
                             mongocxx::options::find_one_and_update options) {
  if (!options.return_document()) {
    options.return_document(mongocxx::options::return_document::k_after);
  }

  return engine::Async(
      impl_->GetPool().GetTaskProcessor(),
      [impl = impl_](auto&& query, auto&& update, const auto& options) {
        auto conn = impl->GetPool().Acquire();
        try {
          return ToBoostOptional(impl->GetCollection(conn).find_one_and_update(
              query.view(), update.view(), options));
        } catch (const mongocxx::write_exception& ex) {
          // Retry duplicate key upsert once, could've been race
          if (IsDuplicateKeyError(ex) && options.upsert() &&
              *options.upsert()) {
            return ToBoostOptional(
                impl->GetCollection(conn).find_one_and_update(
                    query.view(), update.view(), options));
          } else {
            throw;
          }
        }
      },
      std::move(query), std::move(update), std::move(options));
}

engine::TaskWithResult<void> Collection::InsertOne(
    DocumentValue obj, mongocxx::options::insert options) {
  return engine::Async(impl_->GetPool().GetTaskProcessor(),
                       [impl = impl_](auto&& obj, const auto& options) {
                         auto conn = impl->GetPool().Acquire();
                         impl->GetCollection(conn).insert_one(std::move(obj),
                                                              options);
                       },
                       std::move(obj), std::move(options));
}

engine::TaskWithResult<size_t> Collection::Count(
    DocumentValue query, mongocxx::options::count options) const {
  return engine::Async(
      impl_->GetPool().GetTaskProcessor(),
      [impl = impl_](auto&& query, const auto& options)->size_t {
        auto conn = impl->GetPool().Acquire();
        return impl->GetCollection(conn).count(std::move(query), options);
      },
      std::move(query), std::move(options));
}

engine::TaskWithResult<bool> Collection::DeleteOne(
    DocumentValue query, mongocxx::options::delete_options options) {
  return engine::Async(impl_->GetPool().GetTaskProcessor(),
                       [impl = impl_](auto&& query, const auto& options) {
                         auto conn = impl->GetPool().Acquire();
                         auto result = impl->GetCollection(conn).delete_one(
                             std::move(query), options);
                         return result && result->deleted_count();
                       },
                       std::move(query), std::move(options));
}

engine::TaskWithResult<size_t> Collection::DeleteMany(
    DocumentValue query, mongocxx::options::delete_options options) {
  return engine::Async(
      impl_->GetPool().GetTaskProcessor(),
      [impl = impl_](auto&& query, const auto& options)->size_t {
        auto conn = impl->GetPool().Acquire();
        auto result =
            impl->GetCollection(conn).delete_many(std::move(query), options);
        return result ? result->deleted_count() : 0;
      },
      std::move(query), std::move(options));
}

engine::TaskWithResult<bool> Collection::ReplaceOne(
    DocumentValue query, DocumentValue replacement,
    mongocxx::options::update options) {
  return engine::Async(impl_->GetPool().GetTaskProcessor(), [
    impl = impl_, query = std::move(query),
    replacement = std::move(replacement), options = std::move(options)
  ] {
    auto conn = impl->GetPool().Acquire();
    auto result = impl->GetCollection(conn).replace_one(
        query.view(), replacement.view(), options);
    return result && result->matched_count();
  });
}

engine::TaskWithResult<bool> Collection::UpdateOne(
    DocumentValue query, DocumentValue update,
    mongocxx::options::update options) {
  return engine::Async(impl_->GetPool().GetTaskProcessor(), [
    impl = impl_, query = std::move(query), update = std::move(update),
    options = std::move(options)
  ] {
    auto conn = impl->GetPool().Acquire();
    auto result = impl->GetCollection(conn).update_one(query.view(),
                                                       update.view(), options);
    return result && result->matched_count();
  });
}

engine::TaskWithResult<size_t> Collection::UpdateMany(
    DocumentValue query, DocumentValue update,
    mongocxx::options::update options) {
  return engine::Async(
      impl_->GetPool().GetTaskProcessor(),
      [
        impl = impl_, query = std::move(query), update = std::move(update),
        options = std::move(options)
      ]()
          ->size_t {
            auto conn = impl->GetPool().Acquire();
            auto result = impl->GetCollection(conn).update_many(
                query.view(), update.view(), options);
            return result ? result->matched_count() : 0;
          });
}

BulkOperationBuilder Collection::UnorderedBulk() {
  return BulkOperationBuilder(
      std::make_shared<impl::BulkOperationBuilderImpl>(impl_));
}

}  // namespace mongo
}  // namespace storages
