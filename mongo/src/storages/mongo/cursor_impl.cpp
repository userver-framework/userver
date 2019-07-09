#include <storages/mongo/cursor_impl.hpp>

#include <stdexcept>

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <storages/mongo/mongo_error.hpp>

#include <formats/bson/wrappers.hpp>
#include <utils/assert.hpp>

namespace storages::mongo::impl {

CursorImpl::CursorImpl(
    PoolImpl::BoundClientPtr client, CursorPtr cursor,
    std::shared_ptr<stats::ReadOperationStatistics> stats_ptr)
    : client_(std::move(client)),
      cursor_(std::move(cursor)),
      stats_ptr_(std::move(stats_ptr)) {
  Next();  // prime the cursor
}

bool CursorImpl::IsValid() const { return cursor_ || current_; }

bool CursorImpl::HasMore() const {
  return cursor_ && mongoc_cursor_more(cursor_.get());
}

const formats::bson::Document& CursorImpl::Current() const {
  if (!IsValid()) throw std::logic_error("Reading from invalid cursor");
  return *current_;
}

void CursorImpl::Next() {
  if (!IsValid()) throw std::logic_error("Advancing cursor past the end");

  current_ = boost::none;
  if (!HasMore()) {
    UASSERT(!cursor_ && !client_);
    return;
  }

  UASSERT(client_ && cursor_);
  const auto batch_num_before = mongoc_cursor_get_batch_num(cursor_.get());
  stats::OperationStopwatch<stats::ReadOperationStatistics> cursor_next_sw(
      stats_ptr_, batch_num_before == -1
                      ? stats::ReadOperationStatistics::kFind
                      : stats::ReadOperationStatistics::kGetMore);

  const bson_t* current_bson = nullptr;
  MongoError error;
  while (!mongoc_cursor_error(cursor_.get(), error.GetNative()) && HasMore()) {
    if (mongoc_cursor_next(cursor_.get(), &current_bson)) {
      current_ = formats::bson::Document(
          formats::bson::impl::MutableBson::CopyNative(current_bson).Extract());
      break;
    }
  }
  if (batch_num_before == mongoc_cursor_get_batch_num(cursor_.get())) {
    cursor_next_sw.Discard();
  } else if (!error) {
    cursor_next_sw.AccountSuccess();
  } else {
    cursor_next_sw.AccountError(error.GetKind());
  }
  if (!HasMore()) {
    cursor_.reset();
    client_.reset();
  }
  if (error) {
    error.Throw("Error iterating over query results");
  }
}

}  // namespace storages::mongo::impl
