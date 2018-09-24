#include <storages/mongo/cursor.hpp>

#include <cassert>

#include "cursor_impl.hpp"

namespace storages {
namespace mongo {

Cursor::Cursor(std::unique_ptr<impl::CursorImpl>&& impl)
    : impl_(std::move(impl)) {}

Cursor::~Cursor() = default;
Cursor::Cursor(Cursor&&) noexcept = default;
Cursor& Cursor::operator=(Cursor&&) noexcept = default;

Cursor::Iterator Cursor::begin() const { return Iterator(impl_.get()); }
Cursor::Iterator Cursor::end() const { return Iterator(nullptr); }

bool Cursor::More() const { return begin() != end(); }

DocumentValue Cursor::Next() const { return std::move(*begin()++); }

Cursor::Iterator::Iterator(impl::CursorImpl* cursor) : cursor_(cursor) {
  if (cursor_ && cursor_->IsExhausted()) cursor_ = nullptr;
}

Cursor::Iterator::reference Cursor::Iterator::operator*() const {
  assert(cursor_);
  return **cursor_;
}

Cursor::Iterator::pointer Cursor::Iterator::operator->() const {
  assert(cursor_);
  return cursor_->operator->();
}

Cursor::Iterator& Cursor::Iterator::operator++() {
  assert(cursor_);
  ++*cursor_;
  if (cursor_->IsExhausted()) cursor_ = nullptr;
  return *this;
}

std::unique_ptr<DocumentValue> Cursor::Iterator::operator++(int) {
  auto value = std::make_unique<DocumentValue>(**this);
  ++*this;
  return value;
}

bool Cursor::Iterator::operator==(const Iterator& rhs) const {
  return cursor_ == rhs.cursor_;
}

bool Cursor::Iterator::operator!=(const Iterator& rhs) const {
  return !(*this == rhs);
}

}  // namespace mongo
}  // namespace storages
