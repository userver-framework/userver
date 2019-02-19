#include <storages/mongo_ng/cursor.hpp>

#include <storages/mongo_ng/cursor_impl.hpp>

namespace storages::mongo_ng {

Cursor::Cursor(impl::CursorImpl impl) : impl_(std::move(impl)) {}

Cursor::~Cursor() = default;
Cursor::Cursor(Cursor&&) noexcept = default;
Cursor& Cursor::operator=(Cursor&&) noexcept = default;

Cursor::Iterator Cursor::begin() { return Iterator(this); }

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

}  // namespace storages::mongo_ng
