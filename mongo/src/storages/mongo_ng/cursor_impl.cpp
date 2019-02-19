#include <storages/mongo_ng/cursor_impl.hpp>

#include <stdexcept>

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <formats/bson/wrappers.hpp>
#include <storages/mongo_ng/bson_error.hpp>

namespace storages::mongo_ng::impl {

CursorImpl::CursorImpl(PoolImpl::BoundClientPtr client, CursorPtr cursor)
    : client_(std::move(client)), cursor_(std::move(cursor)) {
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
    assert(!cursor_ && !client_);
    return;
  }

  assert(client_ && cursor_);
  const bson_t* current_bson = nullptr;
  impl::BsonError error;
  while (!mongoc_cursor_error(cursor_.get(), error.Get()) && HasMore()) {
    if (mongoc_cursor_next(cursor_.get(), &current_bson)) {
      current_ = formats::bson::Document(
          formats::bson::impl::MutableBson(current_bson).Extract());
      break;
    }
  }
  if (!HasMore()) {
    cursor_.reset();
    client_.reset();
  }
  if (error) error.Throw();
}

}  // namespace storages::mongo_ng::impl
