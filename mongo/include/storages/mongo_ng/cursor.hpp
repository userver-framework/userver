#pragma once

/// @file storages/mongo_ng/cursor.hpp
/// @brief @copybrief storages::mongo_ng::Cursor

#include <iterator>

#include <formats/bson/document.hpp>
#include <utils/fast_pimpl.hpp>

namespace storages::mongo_ng {
namespace impl {
class CursorImpl;
}  // namespace impl

/// Interface for MongoDB query cursors
class Cursor {
 public:
  Cursor(impl::CursorImpl);
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

  Iterator begin();
  Iterator end();

 private:
  class Impl;
  static constexpr size_t kSize = 48;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<impl::CursorImpl, kSize, kAlignment, true> impl_;
};

}  // namespace storages::mongo_ng
