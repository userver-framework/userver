#pragma once

/// @file userver/storages/mongo/cursor.hpp
/// @brief @copybrief storages::mongo::Cursor

#include <iterator>
#include <memory>

#include <userver/formats/bson/document.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {
namespace impl {
class CursorImpl;
}  // namespace impl

/// Interface for MongoDB query cursors
class Cursor {
 public:
  explicit Cursor(std::unique_ptr<impl::CursorImpl>&&);
  ~Cursor();

  Cursor(Cursor&&) noexcept;
  Cursor& operator=(Cursor&&) noexcept;

  class Iterator {
   public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = ptrdiff_t;
    using value_type = formats::bson::Document;
    using reference = const value_type&;
    using pointer = const value_type*;

    explicit Iterator(Cursor*);

    class DocHolder {
     public:
      explicit DocHolder(value_type doc) : doc_(std::move(doc)) {}

      value_type& operator*() { return doc_; }

     private:
      value_type doc_;
    };
    DocHolder operator++(int);
    Iterator& operator++();
    reference operator*() const;
    pointer operator->() const;

    bool operator==(const Iterator&) const;
    bool operator!=(const Iterator&) const;

   private:
    Cursor* cursor_;
  };

  bool HasMore() const;
  explicit operator bool() const { return HasMore(); }

  Iterator begin();
  Iterator end();

 private:
  std::unique_ptr<impl::CursorImpl> impl_;
};

}  // namespace storages::mongo

USERVER_NAMESPACE_END
