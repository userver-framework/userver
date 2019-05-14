#include <storages/mongo/cursor_impl.hpp>

#include <stdexcept>

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <storages/mongo/mongo_error.hpp>

#include <formats/bson/wrappers.hpp>
#include <utils/assert.hpp>

namespace storages::mongo::impl {
namespace {

// Dirty hack until TAXICOMMON-962
// NOTE: END_OF_BATCH may never be observed in between operations
enum class MongocCursorState { kUnprimed, kInBatch, kEndOfBatch, kDone };

struct MongocCursorView {
  char data_[20];
  MongocCursorState state;
};

MongocCursorState GetCursorState(const mongoc_cursor_t* cursor) {
  return reinterpret_cast<const MongocCursorView*>(cursor)->state;
}

}  // namespace

CursorImpl::CursorImpl(
    PoolImpl::BoundClientPtr client, CursorPtr cursor,
    std::shared_ptr<stats::Aggregator<stats::ReadOperationStatistics>>
        stats_agg)
    : client_(std::move(client)),
      cursor_(std::move(cursor)),
      stats_agg_(std::move(stats_agg)) {
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
  stats::OperationStopwatch<stats::ReadOperationStatistics> cursor_next_sw;
  switch (GetCursorState(cursor_.get())) {
    case MongocCursorState::kUnprimed:
      cursor_next_sw = stats::OperationStopwatch(
          stats_agg_, stats::ReadOperationStatistics::kFind);
      break;
    default:;  // do not account
  }

  const bson_t* current_bson = nullptr;
  MongoError error;
  while (!mongoc_cursor_error(cursor_.get(), error.GetNative()) && HasMore()) {
    if (mongoc_cursor_next(cursor_.get(), &current_bson)) {
      current_ = formats::bson::Document(
          formats::bson::impl::MutableBson::CopyNative(current_bson).Extract());
      break;
    }
  }
  if (!error) {
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
