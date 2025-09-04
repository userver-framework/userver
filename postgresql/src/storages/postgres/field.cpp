#include <userver/storages/postgres/field.hpp>

#include <storages/postgres/detail/result_wrapper.hpp>
#include <userver/storages/postgres/result_set.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

//----------------------------------------------------------------------------
// FieldView implementation
//----------------------------------------------------------------------------
io::FieldBuffer FieldView::GetBuffer() const { return res_.GetFieldBuffer(row_index_, field_index_); }

std::string_view FieldView::Name() const { return res_.GetFieldName(field_index_); }

Oid FieldView::GetTypeOid() const { return res_.GetFieldTypeOid(field_index_); }

const io::TypeBufferCategory& FieldView::GetTypeBufferCategories() const { return res_.GetTypeBufferCategories(); }

//----------------------------------------------------------------------------
// Field implementation
//----------------------------------------------------------------------------
FieldDescription Field::Description() const { return res_->GetFieldDescription(field_index_); }

std::string_view Field::Name() const { return res_->GetFieldName(field_index_); }

Field::size_type Field::Length() const { return res_->GetFieldLength(row_index_, field_index_); }

bool Field::IsNull() const { return res_->IsFieldNull(row_index_, field_index_); }

const io::TypeBufferCategory& Field::GetTypeBufferCategories() const { return res_->GetTypeBufferCategories(); }

Oid Field::GetTypeOid() const { return res_->GetFieldTypeOid(field_index_); }

bool Field::IsValid() const { return res_ && row_index_ < res_->RowCount() && field_index_ <= res_->FieldCount(); }

int Field::Compare(const Field& rhs) const { return Distance(rhs); }

std::ptrdiff_t Field::Distance(const Field& rhs) const {
    // Invalid iterators are equal
    if (!IsValid() && !rhs.IsValid()) return 0;
    UASSERT_MSG(res_ == rhs.res_, "Cannot compare iterators in different result sets");
    UASSERT_MSG(row_index_ == rhs.row_index_, "Cannot compare field iterators in different data rows");
    return field_index_ - rhs.field_index_;
}

Field& Field::Advance(std::ptrdiff_t distance) {
    if (IsValid()) {
        // movement is defined only for valid iterator
        const std::ptrdiff_t target = distance + field_index_;
        if (target < 0 || target > static_cast<std::ptrdiff_t>(res_->FieldCount())) {
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

}  // namespace storages::postgres

USERVER_NAMESPACE_END
