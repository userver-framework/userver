#include <storages/mongo/wrappers.hpp>

#include <functional>

#include <engine/async.hpp>

namespace storages {
namespace mongo {

namespace {

bool IsDuplicateKeyError(const ::mongo::BSONObj& obj) {
  return obj.hasField("code") && obj["code"].Int() == 11000;
}

}  // namespace

::mongo::BSONObj BuildFields(std::initializer_list<std::string> fields) {
  ::mongo::BSONObjBuilder builder;
  for (const auto& field : fields) builder.append(field, 1);
  return builder.obj();
}

CursorWrapper::CursorWrapper(Pool::ConnectionPtr&& conn,
                             ::mongo::DBClientCursor* cursor,
                             engine::TaskProcessor& task_processor)
    : conn_(std::move(conn)),
      cursor_(cursor),
      task_processor_(task_processor) {}

bool CursorWrapper::More() const {
  if (!cursor_) throw QueryError("query returned a null cursor");
  return engine::CriticalAsync(task_processor_,
                               [this]() { return cursor_->more(); })
      .Get();
}

::mongo::BSONObj CursorWrapper::Next() const {
  return engine::CriticalAsync(task_processor_,
                               [this]() { return cursor_->next(); })
      .Get();
}

::mongo::BSONObj CursorWrapper::NextSafe() const {
  return engine::CriticalAsync(task_processor_,
                               [this]() { return cursor_->nextSafe(); })
      .Get();
}

BulkOperationBuilder::BulkOperationBuilder(
    Pool::ConnectionPtr&& connection, ::mongo::BulkOperationBuilder&& impl,
    engine::TaskProcessor& task_processor)
    : connection_(std::move(connection)),
      impl_(std::move(impl)),
      task_processor_(task_processor) {}

::mongo::BulkUpdateBuilder BulkOperationBuilder::Find(
    const ::mongo::BSONObj& selector) {
  return engine::CriticalAsync(
             task_processor_,
             [this, &selector]() { return impl_.find(selector); })
      .Get();
}

void BulkOperationBuilder::Insert(const ::mongo::BSONObj& doc) {
  return engine::CriticalAsync(task_processor_,
                               [this, &doc]() { return impl_.insert(doc); })
      .Get();
}

void BulkOperationBuilder::Execute(const ::mongo::WriteConcern* write_concern,
                                   ::mongo::WriteResult* write_result) {
  return engine::CriticalAsync(task_processor_,
                               [this, &write_concern, &write_result]() {
                                 return impl_.execute(write_concern,
                                                      write_result);
                               })
      .Get();
}

CollectionWrapper::CollectionWrapper(std::shared_ptr<Pool> pool,
                                     std::string collection_name,
                                     engine::TaskProcessor& task_processor)
    : pool_(std::move(pool)),
      collection_name_(std::move(collection_name)),
      task_processor_(task_processor) {}

const std::string& CollectionWrapper::GetCollectionName() const {
  return collection_name_;
}

::mongo::BSONObj CollectionWrapper::FindOne(
    const ::mongo::Query& query, const ::mongo::BSONObj* fields) const {
  return Execute<::mongo::BSONObj>(
      [&query, fields](::mongo::DBClientConnection& conn,
                       const std::string& collection_name) {
        return conn.findOne(collection_name, query, fields, 0);
      });
}

::mongo::BSONObj CollectionWrapper::FindOne(
    const ::mongo::Query& query,
    std::initializer_list<std::string> fields) const {
  auto fields_obj = BuildFields(fields);
  return FindOne(query, &fields_obj);
}

::mongo::BSONObj CollectionWrapper::FindOne(
    const ::mongo::BSONObj& query, const ::mongo::BSONObj* fields) const {
  return FindOne(::mongo::Query(query), fields);
}

::mongo::BSONObj CollectionWrapper::FindOne(
    const ::mongo::BSONObj& query,
    std::initializer_list<std::string> fields) const {
  return FindOne(::mongo::Query(query), fields);
}

CursorWrapper CollectionWrapper::Find(const ::mongo::Query& query,
                                      const ::mongo::BSONObj* fields,
                                      ::mongo::QueryOptions options,
                                      int limit) const {
  auto conn = pool_->Acquire();
  try {
    auto cursor = engine::CriticalAsync(
                      task_processor_,
                      [this, &conn, &query, &limit, &fields, &options]() {
                        return conn->query(collection_name_, query, limit, 0,
                                           fields, options);
                      })
                      .Get();
    return CursorWrapper(std::move(conn), cursor.release(), task_processor_);
  } catch (const ::mongo::SocketException&) {
    // HACK(nikslim): code to to force client to reconnect
    auto cursor = engine::CriticalAsync(
                      task_processor_,
                      [this, &conn, &query, &limit, &fields, &options]() {
                        return conn->query(collection_name_, query, limit, 0,
                                           fields, options);
                      })
                      .Get();
    return CursorWrapper(std::move(conn), cursor.release(), task_processor_);
  }
}

CursorWrapper CollectionWrapper::Find(const ::mongo::BSONObj& query,
                                      const ::mongo::BSONObj* fields) const {
  return Find(::mongo::Query(query), fields);
}

CursorWrapper CollectionWrapper::Find(
    const ::mongo::Query& query,
    std::initializer_list<std::string> fields) const {
  auto fields_obj = BuildFields(fields);
  return Find(query, &fields_obj);
}

CursorWrapper CollectionWrapper::Find(
    const ::mongo::BSONObj& query,
    std::initializer_list<std::string> fields) const {
  auto fields_obj = BuildFields(fields);
  return Find(query, &fields_obj);
}

::mongo::BSONObj CollectionWrapper::FindAndModify(
    const ::mongo::BSONObj& query, const ::mongo::BSONObj& update, bool upsert,
    bool return_new, const ::mongo::BSONObj& sort,
    const ::mongo::BSONObj& fields, ::mongo::WriteConcern* wc) const {
  auto conn = pool_->Acquire();
  try {
    return engine::CriticalAsync(task_processor_,
                                 [this, &conn, &query, &update, &upsert,
                                  &return_new, &sort, &fields, &wc]() {
                                   return conn->findAndModify(
                                       collection_name_, query, update, upsert,
                                       return_new, sort, fields, wc);
                                 })
        .Get();
  } catch (const ::mongo::SocketException&) {
    // HACK(nikslim): code to to force client to reconnect
    return engine::CriticalAsync(task_processor_,
                                 [this, &conn, &query, &update, &upsert,
                                  &return_new, &sort, &fields, &wc]() {
                                   return conn->findAndModify(
                                       collection_name_, query, update, upsert,
                                       return_new, sort, fields, wc);
                                 })
        .Get();
  } catch (const ::mongo::OperationException& exc) {
    if (upsert && IsDuplicateKeyError(exc.obj())) {
      return engine::CriticalAsync(task_processor_,
                                   [this, &conn, &query, &update, &upsert,
                                    &return_new, &sort, &fields, &wc]() {
                                     return conn->findAndModify(
                                         collection_name_, query, update,
                                         upsert, return_new, sort, fields, wc);
                                   })
          .Get();
    } else
      throw;
  }
}

::mongo::BSONObj CollectionWrapper::FindAndModify(
    const ::mongo::BSONObj& query, const ::mongo::BSONObj& update, bool upsert,
    bool return_new, const ::mongo::BSONObj& sort,
    std::initializer_list<std::string> fields,
    ::mongo::WriteConcern* wc) const {
  return FindAndModify(query, update, upsert, return_new, sort,
                       BuildFields(fields), wc);
}

::mongo::BSONObj CollectionWrapper::FindAndRemove(
    const ::mongo::BSONObj& query) const {
  return Execute<::mongo::BSONObj>(
      [&query](::mongo::DBClientConnection& conn,
               const std::string& collection_name) {
        return conn.findAndRemove(collection_name, query);
      });
}

size_t CollectionWrapper::Count(const ::mongo::Query& query) const {
  return Execute<size_t>([&query](::mongo::DBClientConnection& conn,
                                  const std::string& collection_name) {
    return conn.count(collection_name, query, 0, 0, 0);
  });
}

size_t CollectionWrapper::Count(const ::mongo::BSONObj& query) const {
  return Count(::mongo::Query(query));
}

void CollectionWrapper::Insert(const ::mongo::BSONObj& object, int flags,
                               const ::mongo::WriteConcern* wc) const {
  return Execute<void>(
      [&object, flags, wc](::mongo::DBClientConnection& conn,
                           const std::string& collection_name) {
        return conn.insert(collection_name, object, flags, wc);
      });
}

void CollectionWrapper::Remove(const ::mongo::Query& query, bool multi) const {
  return Execute<void>([&query, multi](::mongo::DBClientConnection& conn,
                                       const std::string& collection_name) {
    return conn.remove(collection_name, query, !multi);
  });
}

void CollectionWrapper::Remove(const ::mongo::BSONObj& query,
                               bool multi) const {
  return Remove(::mongo::Query(query), multi);
}

void CollectionWrapper::Update(const ::mongo::Query& query,
                               const ::mongo::BSONObj& obj,
                               utils::Flags<UpdateFlags> flags) const {
  return Execute<void>(
      [&query, &obj, flags](::mongo::DBClientConnection& conn,
                            const std::string& collection_name) {
        return conn.update(collection_name, query, obj,
                           !!(flags & UpdateFlags::kUpsert),
                           !!(flags & UpdateFlags::kMulti));
      });
}

void CollectionWrapper::Update(const ::mongo::BSONObj& query,
                               const ::mongo::BSONObj& obj,
                               utils::Flags<UpdateFlags> flags) const {
  return Update(::mongo::Query(query), obj, flags);
}

template <typename Ret, typename Function, typename... Args>
Ret CollectionWrapper::Execute(Function&& function, const Args&... args) const {
  auto conn = pool_->Acquire();
  try {
    return engine::CriticalAsync(
               task_processor_, std::forward<Function>(function),
               std::ref(*conn), std::cref(collection_name_), std::cref(args)...)
        .Get();
  } catch (const ::mongo::SocketException&) {
    /* HACK(nikslim): code to to force client to reconnect:

        src/mongo/client/dbclient.cpp:

            void DBClientConnection::_checkConnection() {
              if (!_failed)
                return;
              ...
            }

            ...

            bool DBClientConnection::call(...) {
              checkConnection();
              try {
                if (!port().call(...)) {
                  _failed = true;
                  ...
                }
              } catch (SocketException&) {
                _failed = true;
                throw;
              }
            }
     */
    return engine::CriticalAsync(
               task_processor_, std::forward<Function>(function),
               std::ref(*conn), std::cref(collection_name_), std::cref(args)...)
        .Get();
  }
}

BulkOperationBuilder CollectionWrapper::UnorderedBulk() const {
  auto conn = pool_->Acquire();
  auto op = conn->initializeUnorderedBulkOp(collection_name_);
  return BulkOperationBuilder(std::move(conn), std::move(op), task_processor_);
}

}  // namespace mongo
}  // namespace storages
