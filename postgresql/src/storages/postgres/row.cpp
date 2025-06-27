#include <userver/storages/postgres/row.hpp>

#include <string_view>

#include <fmt/format.h>

#include <storages/postgres/detail/result_wrapper.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/field.hpp>
#include <userver/storages/postgres/io/user_types.hpp>
#include <userver/storages/postgres/result_set.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

namespace {

// Placeholders for: field name, type class (composite, enum etc), schema, type
// name and oid
constexpr std::string_view kKnownTypeErrorMessageTemplate =
    "PostgreSQL result set field '{}' of a {} type '{}.{}' (oid: {}) doesn't "
    "have a binary parser. There is no C++ type mapped to this database type, "
    "probably you forgot to declare cpp to pg mapping. For more information "
    "see 'Mapping a C++ type to PostgreSQL user type'. Another reason that "
    "can cause such an error is that you modified the mapping to use other "
    "postgres type and forgot to run migration scripts.";

// Placeholders for: field name and oid
constexpr std::string_view kUnknownTypeErrorMessageTemplate =
    "PostgreSQL result set field '{}' has oid {} which was NOT loaded from "
    "database. The type was not loaded due to a migration script creating "
    "the type and probably altering a table was run while the service is up, "
    "the only way to fix this is to restart the service.";

}  // namespace

//----------------------------------------------------------------------------
// RowDescription implementation
//----------------------------------------------------------------------------
void RowDescription::CheckBinaryFormat(const UserTypes& types) const {
    for (std::size_t i = 0; i < res_->FieldCount(); ++i) {
        auto oid = res_->GetFieldTypeOid(i);
        if (!io::HasParser(static_cast<io::PredefinedOids>(oid)) && !types.HasParser(oid)) {
            const auto* desc = types.GetTypeDescription(oid);
            std::string message;
            if (desc) {
                message = fmt::format(
                    kKnownTypeErrorMessageTemplate,
                    res_->GetFieldName(i),
                    ToString(desc->type_class),
                    desc->schema,
                    desc->name,
                    oid
                );
            } else {
                message = fmt::format(kUnknownTypeErrorMessageTemplate, res_->GetFieldName(i), oid);
            }
            throw NoBinaryParser{std::move(message)};
        }
    }
}

//----------------------------------------------------------------------------
// Row implementation
//----------------------------------------------------------------------------
Row::size_type Row::Size() const { return res_->FieldCount(); }

Row::const_iterator Row::cbegin() const { return {res_, row_index_, 0}; }

Row::const_iterator Row::cend() const { return {res_, row_index_, Size()}; }

Row::const_reverse_iterator Row::crbegin() const { return {res_, row_index_, Size() - 1}; }

Row::const_reverse_iterator Row::crend() const { return {res_, row_index_, ResultSet::npos}; }

Row::reference Row::operator[](size_type index) const {
    if (index >= Size()) throw FieldIndexOutOfBounds{index};
    return {res_, row_index_, index};
}

Row::reference Row::operator[](USERVER_NAMESPACE::utils::zstring_view name) const {
    auto idx = IndexOfName(name);
    if (idx == ResultSet::npos) throw FieldNameDoesntExist{name};
    return (*this)[idx];
}
bool Row::IsValid() const { return res_ && row_index_ <= res_->RowCount(); }

int Row::Compare(const Row& rhs) const { return Distance(rhs); }

std::ptrdiff_t Row::Distance(const Row& rhs) const {
    // Invalid iterators are equal
    if (!IsValid() && !rhs.IsValid()) return 0;
    UASSERT_MSG(res_ == rhs.res_, "Cannot compare iterators in different result sets");
    return row_index_ - rhs.row_index_;
}

Row& Row::Advance(std::ptrdiff_t distance) {
    if (IsValid()) {
        // movement is defined only for valid iterators
        const std::ptrdiff_t target = distance + row_index_;
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

Row::size_type Row::IndexOfName(USERVER_NAMESPACE::utils::zstring_view name) const { return res_->IndexOfName(name); }

FieldView Row::GetFieldView(size_type index) const { return FieldView{*res_, row_index_, index}; }

}  // namespace storages::postgres

USERVER_NAMESPACE_END
