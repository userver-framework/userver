#include <storages/postgres/result_set.hpp>

#include <storages/postgres/detail/result_set_impl.hpp>
#include <storages/postgres/exceptions.hpp>

#include <cassert>

namespace storages {
namespace postgres {

//----------------------------------------------------------------------------
// Field implementation
//----------------------------------------------------------------------------
bool Field::IsNull() const {
  return res_->IsFieldNull(row_index_, field_index_);
}

io::FieldBuffer Field::GetBuffer() const {
  return res_->GetFieldBuffer(row_index_, field_index_);
}

//----------------------------------------------------------------------------
// ConstFieldIterator implementation
//----------------------------------------------------------------------------
ConstFieldIterator& ConstFieldIterator::Advance(difference_type distance) {
  if (*this) {
    // movement is defined only for valid iterator
    difference_type target = distance + field_index_;
    if (target < 0 ||
        target > static_cast<difference_type>(res_->FieldCount())) {
      // Invalidate the iterator
      field_index_ = ResultSet::npos;
    } else {
      field_index_ = target;
    }
  } else if (res_) {
    if (distance == 1) {
      // When a non-valid iterator that belongs to a result set
      // is incremented it is moved to the beginning of sequence.
      // This is to support rend iterator moving
      // to the beginning of sequence.
      field_index_ = 0;
    } else if (distance == -1) {
      // When a non-valid iterator that belongs to a result set
      // is decremented it is moved to the end of sequence.
      // This is to support end iterator moving
      // to the end of sequence.
      field_index_ = res_->FieldCount() - 1;
    }
  }
  return *this;
}

int ConstFieldIterator::Compare(const ConstFieldIterator& rhs) const {
  return Distance(rhs);
}

ConstFieldIterator::difference_type ConstFieldIterator::Distance(
    const ConstFieldIterator& rhs) const {
  // Invalid iterators are equal
  if (!*this && !rhs) return 0;
  assert(res_ == rhs.res_ &&
         "Cannot compare iterators in different result sets");
  assert(row_index_ == rhs.row_index_ &&
         "Cannot compare field iterators in different data rows");
  return field_index_ - rhs.field_index_;
}

bool ConstFieldIterator::IsValid() const {
  return res_ && row_index_ < res_->RowCount() &&
         field_index_ <= res_->FieldCount();
}

//----------------------------------------------------------------------------
// Row implementation
//----------------------------------------------------------------------------
Row::size_type Row::Size() const { return res_->FieldCount(); }

Row::const_iterator Row::cbegin() const {
  return const_iterator(res_, row_index_, 0);
}

Row::const_iterator Row::cend() const {
  return const_iterator(res_, row_index_, Size());
}

Row::const_reverse_iterator Row::crbegin() const {
  return const_reverse_iterator(const_iterator(res_, row_index_, Size() - 1));
}

Row::const_reverse_iterator Row::crend() const {
  return const_reverse_iterator(
      const_iterator(res_, row_index_, ResultSet::npos));
}

Row::reference Row::operator[](size_type index) const {
  return reference(res_, row_index_, index);
}

Row::reference Row::At(size_type index) const {
  if (index >= Size()) throw FieldIndexOutOfBounds{index};
  return reference(res_, row_index_, index);
}

Row::reference Row::operator[](const std::string& name) const {
  auto idx = res_->IndexOfName(name);
  if (idx == ResultSet::npos) throw FieldNameDoesntExist{name};
  return (*this)[idx];
}

//----------------------------------------------------------------------------
// ConstRowIterator implementation
//----------------------------------------------------------------------------
ConstRowIterator& ConstRowIterator::Advance(difference_type distance) {
  if (*this) {
    // movement is defined only for valid iterators
    difference_type target = distance + row_index_;
    if (target < 0 || target > static_cast<difference_type>(res_->RowCount())) {
      row_index_ = ResultSet::npos;
    } else {
      row_index_ = target;
    }
  } else if (res_) {
    if (distance == 1) {
      // When a non-valid iterator that belongs to a result set
      // is incremented it is moved to the beginning of sequence.
      // This is to support rend iterator moving
      // to the beginning of sequence.
      row_index_ = 0;
    } else if (distance == -1) {
      // When a non-valid iterator that belongs to a result set
      // is decremented it is moved to the end of sequence.
      // This is to support end iterator moving
      // to the end of sequence.
      row_index_ = res_->RowCount() - 1;
    }
  }
  return *this;
}

int ConstRowIterator::Compare(const ConstRowIterator& rhs) const {
  return Distance(rhs);
}

ConstRowIterator::difference_type ConstRowIterator::Distance(
    const ConstRowIterator& rhs) const {
  // Invalid iterators are equal
  if (!*this && !rhs) return 0;
  assert(res_ == rhs.res_ &&
         "Cannot compare iterators in different result sets");
  return row_index_ - rhs.row_index_;
}

bool ConstRowIterator::IsValid() const {
  return res_ && row_index_ <= res_->RowCount();
}

//----------------------------------------------------------------------------
// ResultSet implementation
//----------------------------------------------------------------------------
ResultSet::size_type ResultSet::Size() const { return pimpl_->RowCount(); }

ResultSet::size_type ResultSet::FieldCount() const {
  return pimpl_->FieldCount();
}

ResultSet::const_iterator ResultSet::cbegin() const {
  return const_iterator(pimpl_, 0);
}

ResultSet::const_iterator ResultSet::cend() const {
  return const_iterator(pimpl_, Size());
}

ResultSet::const_reverse_iterator ResultSet::crbegin() const {
  return const_reverse_iterator(const_iterator(pimpl_, Size() - 1));
}

ResultSet::const_reverse_iterator ResultSet::crend() const {
  return const_reverse_iterator(const_iterator(pimpl_, npos));
}

ResultSet::reference ResultSet::Front() const { return reference(pimpl_, 0); }

ResultSet::reference ResultSet::Back() const {
  return reference(pimpl_, Size() - 1);
}

ResultSet::reference ResultSet::operator[](size_type index) const {
  return reference(pimpl_, index);
}

ResultSet::reference ResultSet::At(size_type index) const {
  if (index >= Size()) throw RowIndexOutOfBounds{index};
  return reference(pimpl_, index);
}

}  // namespace postgres
}  // namespace storages
