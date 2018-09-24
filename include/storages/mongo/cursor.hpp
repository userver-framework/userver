#pragma once

#include <iterator>
#include <memory>

#include <bsoncxx/document/view.hpp>

#include <storages/mongo/mongo.hpp>

namespace storages {
namespace mongo {

namespace impl {
class CursorImpl;
}  // namespace impl

class Cursor {
 public:
  class Iterator {
   public:
    using value_type = bsoncxx::document::view;
    using difference_type = ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = std::input_iterator_tag;

    reference operator*() const;
    pointer operator->() const;

    Iterator& operator++();
    std::unique_ptr<DocumentValue> operator++(int);

    bool operator==(const Iterator&) const;
    bool operator!=(const Iterator&) const;

   private:
    friend class Cursor;
    explicit Iterator(impl::CursorImpl*);

    impl::CursorImpl* cursor_;
  };

  explicit Cursor(std::unique_ptr<impl::CursorImpl>&&);
  ~Cursor();

  Cursor(Cursor&&) noexcept;
  Cursor& operator=(Cursor&&) noexcept;

  Iterator begin() const;
  Iterator end() const;

  bool More() const;
  DocumentValue Next() const;

 private:
  std::unique_ptr<impl::CursorImpl> impl_;
};

}  // namespace mongo
}  // namespace storages
