#include <storages/mongo/cdriver/cursor_impl.hpp>

#include <stdexcept>

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <userver/storages/mongo/mongo_error.hpp>
#include <userver/utils/assert.hpp>

#include <formats/bson/wrappers.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

CDriverCursorImpl::CDriverCursorImpl(
    cdriver::CDriverPoolImpl::BoundClientPtr client,
    cdriver::CursorPtr cursor,
    std::shared_ptr<stats::OperationStatisticsItem> find_stats
)
    : client_(std::move(client)), cursor_(std::move(cursor)), find_stats_(std::move(find_stats)) {
    if (cursor_) {
        // Precondition: we've got a valid cursor (it could be errored-out right
        // away due to stream selection error, for example).
        MongoError error;
        if (mongoc_cursor_error(cursor_.get(), error.GetNative())) {
            error.Throw("Error iterating over query results");
        }
    }

    // Prime the cursor
    Next();
}

bool CDriverCursorImpl::IsValid() const { return cursor_ || current_; }

bool CDriverCursorImpl::HasMore() const { return cursor_ && mongoc_cursor_more(cursor_.get()); }

const formats::bson::Document& CDriverCursorImpl::Current() const {
    if (!IsValid()) throw std::logic_error("Reading from invalid cursor");
    return *current_;
}

void CDriverCursorImpl::Next() {
    if (!IsValid()) throw std::logic_error("Advancing cursor past the end");

    current_ = std::nullopt;
    if (!HasMore()) {
        UASSERT(!cursor_ && !client_);
        return;
    }

    UASSERT(client_ && cursor_);
    const auto before_stats = client_.GetEventStatsSnapshot();
    stats::OperationStopwatch cursor_next_sw(find_stats_, "find");

    const bson_t* current_bson = nullptr;
    MongoError error;
    while (!mongoc_cursor_error(cursor_.get(), error.GetNative()) && HasMore()) {
        if (mongoc_cursor_next(cursor_.get(), &current_bson)) {
            current_ = formats::bson::Document(formats::bson::impl::MutableBson::CopyNative(current_bson).Extract());
            break;
        }
    }
    if (before_stats == client_.GetEventStatsSnapshot()) {
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

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
