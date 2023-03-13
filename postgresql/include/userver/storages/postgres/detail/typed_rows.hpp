#pragma once

#include <boost/pfr/core.hpp>

#include <userver/storages/postgres/result_set.hpp>

#include <userver/storages/postgres/detail/iterator_direction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

template <typename T, typename ExtractionTag,
          IteratorDirection direction = IteratorDirection::kForward>
class ConstTypedRowIterator : private Row {
 public:
  //@{
  /** @name Iterator concept */
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using reference = value_type;
  using pointer = const value_type*;
  /// This iterator doesn't satisfy ForwardIterator concept as it returns
  /// data by value. In all other respects it behaves as a random access
  /// iterator.
  using iterator_category = std::input_iterator_tag;
  //@}
  static constexpr ExtractionTag kExtractTag{};

  //@{
  /** @name Iterator dereferencing */
  /// Read typed value from underlying postgres buffers and return it.
  /// Please note there is no operator ->.
  value_type operator*() const { return As<value_type>(kExtractTag); }
  //@}
  //@{
  /** @name Iterator validity */
  explicit operator bool() const { return this->IsValid(); }
  bool operator!() const { return !this->IsValid(); }
  //@}

  //@{
  /** @name Iterator movement */
  ConstTypedRowIterator& operator++() { return DoAdvance(1); }
  ConstTypedRowIterator operator++(int) {
    ConstTypedRowIterator prev{*this};
    DoAdvance(1);
    return prev;
  }

  ConstTypedRowIterator& operator--() { return DoAdvance(-1); }
  ConstTypedRowIterator operator--(int) {
    ConstTypedRowIterator prev{*this};
    DoAdvance(-1);
    return prev;
  }

  ConstTypedRowIterator operator+(difference_type distance) const {
    ConstTypedRowIterator res{*this};
    res.DoAdvance(distance);
    return res;
  }
  ConstTypedRowIterator operator-(difference_type distance) const {
    ConstTypedRowIterator res{*this};
    res.DoAdvance(-distance);
    return res;
  }

  difference_type operator-(const ConstTypedRowIterator& rhs) const {
    return this->Distance(rhs) * static_cast<int>(direction);
  }

  ConstTypedRowIterator operator[](difference_type index) const {
    return *this + index * static_cast<int>(direction);
  }

  ConstTypedRowIterator& operator+=(difference_type distance) {
    return DoAdvance(distance);
  }
  ConstTypedRowIterator& operator-=(difference_type distance) {
    return DoAdvance(-distance);
  }
  //@}

  //@{
  /** @name Iterator comparison */
  bool operator==(const ConstTypedRowIterator& rhs) const {
    return this->Compare(rhs) == 0;
  }

  bool operator!=(const ConstTypedRowIterator& rhs) const {
    return !(*this == rhs);
  }

  bool operator<(const ConstTypedRowIterator& rhs) const {
    return this->Compare(rhs) < 0;
  }
  bool operator<=(const ConstTypedRowIterator& rhs) const {
    return this->Compare(rhs) <= 0;
  }
  bool operator>(const ConstTypedRowIterator& rhs) const {
    return this->Compare(rhs) > 0;
  }
  bool operator>=(const ConstTypedRowIterator& rhs) const {
    return this->Compare(rhs) >= 0;
  }
  //@}
 private:
  friend class TypedResultSet<T, ExtractionTag>;

  ConstTypedRowIterator(detail::ResultWrapperPtr res, size_type row)
      : Row{std::move(res), row} {}
  ConstTypedRowIterator& DoAdvance(difference_type distance) {
    this->Advance(distance * static_cast<int>(direction));
    return *this;
  }
};

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
