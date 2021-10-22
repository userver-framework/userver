#pragma once

#include <iterator>

USERVER_NAMESPACE_BEGIN

namespace storages {
namespace postgres {
namespace detail {

/// @brief Template for implementing ResultSet::ConstRowIterator and
/// ResultSet::ConstFieldIterator.
///
/// FinalType must provide following public functions:
/// Advance(int)
/// Compare(...)
/// Distance(...)
/// Valid()
template <typename FinalType, typename DataType>
class ConstDataIterator : protected DataType {
 public:
  //@{
  /** @name Iterator concept */
  using value_type = DataType;
  using difference_type = std::ptrdiff_t;
  using reference = const value_type&;
  using pointer = const value_type*;
  using iterator_category = std::random_access_iterator_tag;
  //@}
 public:
  //@{
  /** @name Iterator dereferencing */
  reference operator*() const { return static_cast<reference>(*this); }
  pointer operator->() const { return static_cast<pointer>(this); }
  //@}

  //@{
  /** @name Iterator validity */
  explicit operator bool() const { return this->IsValid(); }
  bool operator!() const { return !this->IsValid(); }
  //@}

  //@{
  /** @name Iterator movement */
  FinalType& operator++() { return DoAdvance(1); }
  FinalType operator++(int) {
    FinalType prev{Rebind()};
    DoAdvance(1);
    return prev;
  }

  FinalType& operator--() { return DoAdvance(-1); }
  FinalType operator--(int) {
    FinalType prev{Rebind()};
    DoAdvance(-1);
    return prev;
  }

  FinalType operator+(difference_type distance) const {
    FinalType res{Rebind()};
    res.DoAdvance(distance);
    return res;
  }

  FinalType operator-(difference_type distance) const {
    FinalType res{Rebind()};
    res.DoAdvance(-distance);
    return res;
  }
  difference_type operator-(const FinalType& rhs) const {
    return this->Distance(rhs);
  }

  FinalType operator[](difference_type index) const { return *this + index; }

  FinalType& operator+=(difference_type distance) {
    return DoAdvance(distance);
  }
  FinalType& operator-=(difference_type distance) {
    return DoAdvance(-distance);
  }
  //@}

  //@{
  /** @name Iterator comparison */
  bool operator==(const FinalType& rhs) const { return DoCompare(rhs) == 0; }

  bool operator!=(const FinalType& rhs) const { return !(*this == rhs); }

  bool operator<(const FinalType& rhs) const { return DoCompare(rhs) < 0; }
  bool operator<=(const FinalType& rhs) const { return DoCompare(rhs) <= 0; }
  bool operator>(const FinalType& rhs) const { return DoCompare(rhs) > 0; }
  bool operator>=(const FinalType& rhs) const { return DoCompare(rhs) >= 0; }
  //@}
 protected:
  template <typename... T>
  ConstDataIterator(T&&... args) : value_type(std::forward<T>(args)...) {}

 private:
  FinalType& Rebind() { return static_cast<FinalType&>(*this); }
  const FinalType& Rebind() const {
    return static_cast<const FinalType&>(*this);
  }

  FinalType& DoAdvance(difference_type distance) {
    this->Advance(distance);
    return Rebind();
  }
  int DoCompare(const FinalType& lhs) const { return this->Compare(lhs); }
};

}  // namespace detail
}  // namespace postgres
}  // namespace storages

USERVER_NAMESPACE_END
