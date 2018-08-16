#pragma once

#include <initializer_list>
#include <memory>
#include <string>

#include <mongo/bson/bson.h>
#include <mongo/client/dbclient.h>

#include <engine/task/task_processor.hpp>
#include <utils/flags.hpp>

#include "mongo.hpp"
#include "pool.hpp"

namespace storages {
namespace mongo {

class QueryError : public MongoError {
  using MongoError::MongoError;
};

enum class UpdateFlags {
  kNone = 0,
  kUpsert = (1 << 0),
  kMulti = (1 << 1),
};

::mongo::BSONObj BuildFields(std::initializer_list<std::string> fields);

// Wrapper around ::mongo::DbClientCursor that owns Pool::ConnectionPtr
class CursorWrapper {
 public:
  CursorWrapper(Pool::ConnectionPtr&& conn, ::mongo::DBClientCursor* cursor,
                engine::TaskProcessor& task_processor);

  bool More() const;
  ::mongo::BSONObj Next() const;
  ::mongo::BSONObj NextSafe() const;

 private:
  Pool::ConnectionPtr conn_;
  std::unique_ptr<::mongo::DBClientCursor> cursor_;
  engine::TaskProcessor& task_processor_;
};

class BulkOperationBuilder {
 public:
  BulkOperationBuilder(Pool::ConnectionPtr&& connection,
                       ::mongo::BulkOperationBuilder&& impl,
                       engine::TaskProcessor& task_processor);
  ::mongo::BulkUpdateBuilder Find(const ::mongo::BSONObj& selector);
  void Insert(const ::mongo::BSONObj& doc);
  void Execute(const ::mongo::WriteConcern* write_concern,
               ::mongo::WriteResult* write_result);

 private:
  Pool::ConnectionPtr connection_;
  ::mongo::BulkOperationBuilder impl_;
  engine::TaskProcessor& task_processor_;
};

// Wrapper that keeps owning on Pool instance
class CollectionWrapper {
 public:
  CollectionWrapper(std::shared_ptr<Pool> pool, std::string collection_name,
                    engine::TaskProcessor& task_processor);

  const std::string& GetCollectionName() const;

  ::mongo::BSONObj FindOne(const ::mongo::Query& query,
                           const ::mongo::BSONObj* fields = nullptr) const;
  ::mongo::BSONObj FindOne(const ::mongo::Query& query,
                           std::initializer_list<std::string> fields) const;
  ::mongo::BSONObj FindOne(const ::mongo::BSONObj& query,
                           const ::mongo::BSONObj* fields = nullptr) const;
  ::mongo::BSONObj FindOne(const ::mongo::BSONObj& query,
                           std::initializer_list<std::string> fields) const;

  CursorWrapper Find(const ::mongo::Query& query,
                     const ::mongo::BSONObj* fields = nullptr,
                     ::mongo::QueryOptions options = {}, int limit = 0) const;
  CursorWrapper Find(const ::mongo::BSONObj& query,
                     const ::mongo::BSONObj* fields = nullptr) const;
  CursorWrapper Find(const ::mongo::Query& query,
                     std::initializer_list<std::string> fields) const;
  CursorWrapper Find(const ::mongo::BSONObj& query,
                     std::initializer_list<std::string> fields) const;

  ::mongo::BSONObj FindAndModify(const ::mongo::BSONObj& query,
                                 const ::mongo::BSONObj& update,
                                 bool upsert = false, bool returnNew = true,
                                 const ::mongo::BSONObj& sort = kEmptyObject,
                                 const ::mongo::BSONObj& fields = kEmptyObject,
                                 ::mongo::WriteConcern* wc = nullptr) const;
  ::mongo::BSONObj FindAndModify(const ::mongo::BSONObj& query,
                                 const ::mongo::BSONObj& update, bool upsert,
                                 bool returnNew, const ::mongo::BSONObj& sort,
                                 std::initializer_list<std::string> fields,
                                 ::mongo::WriteConcern* wc = nullptr) const;
  ::mongo::BSONObj FindAndRemove(const ::mongo::BSONObj& query) const;

  void Insert(const ::mongo::BSONObj& object, int flags = 0,
              const ::mongo::WriteConcern* wc = nullptr) const;

  size_t Count(const ::mongo::Query& query) const;
  size_t Count(const ::mongo::BSONObj& query) const;

  void Remove(const ::mongo::Query& query, bool multi = false) const;
  void Remove(const ::mongo::BSONObj& query, bool multi = false) const;

  void Update(const ::mongo::Query& query, const ::mongo::BSONObj& obj,
              utils::Flags<UpdateFlags> flags = {}) const;
  void Update(const ::mongo::BSONObj& query, const ::mongo::BSONObj& obj,
              utils::Flags<UpdateFlags> flags = {}) const;

  BulkOperationBuilder UnorderedBulk() const;

 private:
  std::shared_ptr<Pool> pool_;
  std::string collection_name_;
  engine::TaskProcessor& task_processor_;

  template <typename Ret, typename Function, typename... Args>
  Ret Execute(Function&& function, const Args&... args) const;
};

}  // namespace mongo
}  // namespace storages
