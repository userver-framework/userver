#include <userver/storages/mongo/cursor.hpp>

#include <storages/mongo/cursor_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

Cursor::Cursor(std::unique_ptr<impl::CursorImpl>&& impl)
    : impl_(std::move(impl)) {}

Cursor::~Cursor() = default;
Cursor::Cursor(Cursor&&) noexcept = default;
Cursor& Cursor::operator=(Cursor&&) noexcept = default;

bool Cursor::HasMore() const { return impl_->IsValid(); }

Cursor::Iterator Cursor::begin() { return Iterator(this); }

// no, part of the iterator interface
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Cursor::Iterator Cursor::end() { return Iterator(nullptr); }

Cursor::Iterator::Iterator(Cursor* cursor) : cursor_(cursor) {
  if (cursor_ && !cursor_->impl_->IsValid()) cursor_ = nullptr;
}

Cursor::Iterator::DocHolder Cursor::Iterator::operator++(int) {
  DocHolder old_value(**this);
  ++*this;
  return old_value;
}

Cursor::Iterator& Cursor::Iterator::operator++() {
  cursor_->impl_->Next();
  if (!cursor_->impl_->IsValid()) cursor_ = nullptr;
  return *this;
}

const formats::bson::Document& Cursor::Iterator::operator*() const {
  return cursor_->impl_->Current();
}

const formats::bson::Document* Cursor::Iterator::operator->() const {
  return &cursor_->impl_->Current();
}

bool Cursor::Iterator::operator==(const Iterator& rhs) const {
  return cursor_ == rhs.cursor_;
}

bool Cursor::Iterator::operator!=(const Iterator& rhs) const {
  return !(*this == rhs);
}

}  // namespace storages::mongo

USERVER_NAMESPACE_END
