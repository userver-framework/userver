#pragma once

/// @file storages/mongo/cursor.hpp
/// @brief @copybrief storages::mongo::Cursor

#include <iterator>
#include <memory>

#include <bsoncxx/document/view.hpp>

#include <storages/mongo/mongo.hpp>

namespace storages {
namespace mongo {

namespace impl {
class CursorImpl;
}  // namespace impl

/// Database cursor interface
class Cursor {
 public:
  /// Iterator for range-based loops
  class Iterator {
   public:
    using value_type = bsoncxx::document::view;
    using difference_type = ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = std::input_iterator_tag;

    /// @{
    /// Accesses the current document
    reference operator*() const;
    pointer operator->() const;
    /// @}

    /// @{
    /// @brief Retrieves the next document
    /// @note This is a blocking operation!
    Iterator& operator++();
    std::unique_ptr<DocumentValue> operator++(int);
    /// @}

    /// @{
    /// Compares two iterators
    bool operator==(const Iterator&) const;
    bool operator!=(const Iterator&) const;
    /// @}

   private:
    friend class Cursor;
    explicit Iterator(impl::CursorImpl*);

    impl::CursorImpl* cursor_;
  };

  /// @cond
  /// Constructor for internal use
  explicit Cursor(std::unique_ptr<impl::CursorImpl>&&);
  /// @endcond

  ~Cursor();

  Cursor(Cursor&&) noexcept;
  Cursor& operator=(Cursor&&) noexcept;

  /// Returns an iterator pointing at the current document
  Iterator begin() const;

  /// Returns an iterator corresponding to an exhausted cursor
  Iterator end() const;

  /// Returns whether more results are available
  bool More() const;

  /// @brief Returns a copy of the current document and advances the cursor
  /// @note This is a blocking operation!
  DocumentValue Next() const;

 private:
  std::unique_ptr<impl::CursorImpl> impl_;
};

}  // namespace mongo
}  // namespace storages
