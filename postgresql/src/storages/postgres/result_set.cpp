#include <userver/storages/postgres/result_set.hpp>

#include <string_view>

#include <fmt/format.h>

#include <storages/postgres/detail/result_wrapper.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/user_types.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

namespace {

// Placeholders for: field name, type class (composite, enum etc), schema, type
// name and oid
constexpr std::string_view kKnownTypeErrorMessageTemplate =
    "PostgreSQL result set field `{}` {} type {}.{} oid {} doesn't have a "
    "binary parser. There is no C++ type mapped to this database type, "
    "probably you forgot to declare cpp to pg mapping. For more information "
    "see 'Mapping a C++ type to PostgreSQL user type'. Another reason that "
    "can cause such an error is that you modified the mapping to use other "
    "postgres type and forgot to run migration scripts.";

// Placeholders for: field name and oid
constexpr std::string_view kUnknownTypeErrorMessageTemplate =
    "PostgreSQL result set field `{}` type with oid {} was NOT loaded from "
    "database. The type was not loaded due to a migration script creating "
    "the type and probably altering a table was run while service up, the only "
    "way to fix this is to restart the service.";

}  // namespace

//----------------------------------------------------------------------------
// RowDescription implementation
//----------------------------------------------------------------------------
void RowDescription::CheckBinaryFormat(const UserTypes& types) const {
  for (std::size_t i = 0; i < res_->FieldCount(); ++i) {
    auto oid = res_->GetFieldTypeOid(i);
    if (!io::HasParser(static_cast<io::PredefinedOids>(oid)) &&
        !types.HasParser(oid)) {
      const auto* desc = types.GetTypeDescription(oid);
      std::string message;
      if (desc) {
        message = fmt::format(kKnownTypeErrorMessageTemplate,
                              res_->GetFieldName(i), ToString(desc->type_class),
                              desc->schema, desc->name, oid);
      } else {
        message = fmt::format(kUnknownTypeErrorMessageTemplate,
                              res_->GetFieldName(i), oid);
      }
      throw NoBinaryParser{message};
    }
  }
}

//----------------------------------------------------------------------------
// Field implementation
//----------------------------------------------------------------------------
FieldDescription Field::Description() const {
  return res_->GetFieldDescription(field_index_);
}

std::string_view Field::Name() const {
  return res_->GetFieldName(field_index_);
}

bool Field::IsNull() const {
  return res_->IsFieldNull(row_index_, field_index_);
}

io::FieldBuffer Field::GetBuffer() const {
  return res_->GetFieldBuffer(row_index_, field_index_);
}

const io::TypeBufferCategory& Field::GetTypeBufferCategories() const {
  return res_->GetTypeBufferCategories();
}

Oid Field::GetTypeOid() const { return res_->GetFieldTypeOid(field_index_); }

bool Field::IsValid() const {
  return res_ && row_index_ < res_->RowCount() &&
         field_index_ <= res_->FieldCount();
}

int Field::Compare(const Field& rhs) const { return Distance(rhs); }

std::ptrdiff_t Field::Distance(const Field& rhs) const {
  // Invalid iterators are equal
  if (!IsValid() && !rhs.IsValid()) return 0;
  UASSERT_MSG(res_ == rhs.res_,
              "Cannot compare iterators in different result sets");
  UASSERT_MSG(row_index_ == rhs.row_index_,
              "Cannot compare field iterators in different data rows");
  return field_index_ - rhs.field_index_;
}

Field& Field::Advance(std::ptrdiff_t distance) {
  if (IsValid()) {
    // movement is defined only for valid iterator
    std::ptrdiff_t target = distance + field_index_;
    if (target < 0 ||
        target > static_cast<std::ptrdiff_t>(res_->FieldCount())) {
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

//----------------------------------------------------------------------------
// Row implementation
//----------------------------------------------------------------------------
Row::size_type Row::Size() const { return res_->FieldCount(); }

Row::const_iterator Row::cbegin() const { return {res_, row_index_, 0}; }

Row::const_iterator Row::cend() const { return {res_, row_index_, Size()}; }

Row::const_reverse_iterator Row::crbegin() const {
  return {res_, row_index_, Size() - 1};
}

Row::const_reverse_iterator Row::crend() const {
  return {res_, row_index_, ResultSet::npos};
}

Row::reference Row::operator[](size_type index) const {
  if (index >= Size()) throw FieldIndexOutOfBounds{index};
  return {res_, row_index_, index};
}

Row::reference Row::operator[](const std::string& name) const {
  auto idx = IndexOfName(name);
  if (idx == ResultSet::npos) throw FieldNameDoesntExist{name};
  return (*this)[idx];
}
bool Row::IsValid() const { return res_ && row_index_ <= res_->RowCount(); }

int Row::Compare(const Row& rhs) const { return Distance(rhs); }

std::ptrdiff_t Row::Distance(const Row& rhs) const {
  // Invalid iterators are equal
  if (!IsValid() && !rhs.IsValid()) return 0;
  UASSERT_MSG(res_ == rhs.res_,
              "Cannot compare iterators in different result sets");
  return row_index_ - rhs.row_index_;
}

Row& Row::Advance(std::ptrdiff_t distance) {
  if (IsValid()) {
    // movement is defined only for valid iterators
    std::ptrdiff_t target = distance + row_index_;
    if (target < 0 || target > static_cast<std::ptrdiff_t>(res_->RowCount())) {
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

//----------------------------------------------------------------------------
// ResultSet implementation
//----------------------------------------------------------------------------
ResultSet::size_type ResultSet::Size() const {
  return pimpl_ != nullptr ? pimpl_->RowCount() : 0;
}

ResultSet::size_type ResultSet::FieldCount() const {
  return pimpl_->FieldCount();
}

ResultSet::size_type ResultSet::RowsAffected() const {
  return pimpl_->RowsAffected();
}

std::string ResultSet::CommandStatus() const { return pimpl_->CommandStatus(); }

ResultSet::const_iterator ResultSet::cbegin() const& { return {pimpl_, 0}; }

ResultSet::const_iterator ResultSet::cend() const& { return {pimpl_, Size()}; }

ResultSet::const_reverse_iterator ResultSet::crbegin() const& {
  return {pimpl_, Size() - 1};
}

ResultSet::const_reverse_iterator ResultSet::crend() const& {
  return {pimpl_, npos};
}

ResultSet::reference ResultSet::Front() const& { return (*this)[0]; }

ResultSet::reference ResultSet::Back() const& { return (*this)[Size() - 1]; }

ResultSet::reference ResultSet::operator[](size_type index) const& {
  if (index >= Size()) throw RowIndexOutOfBounds{index};
  return {pimpl_, index};
}

void ResultSet::FillBufferCategories(const UserTypes& types) {
  pimpl_->FillBufferCategories(types);
}

void ResultSet::SetBufferCategoriesFrom(const ResultSet& dsc) {
  pimpl_->SetTypeBufferCategories(dsc.pimpl_->GetTypeBufferCategories());
}

Row::size_type Row::IndexOfName(const std::string& name) const {
  return res_->IndexOfName(name);
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
