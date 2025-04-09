#include <userver/storages/postgres/result_set.hpp>

#include <storages/postgres/detail/result_wrapper.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/user_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

//----------------------------------------------------------------------------
// ResultSet implementation
//----------------------------------------------------------------------------
ResultSet::size_type ResultSet::Size() const { return pimpl_ != nullptr ? pimpl_->RowCount() : 0; }

ResultSet::size_type ResultSet::FieldCount() const { return pimpl_->FieldCount(); }

ResultSet::size_type ResultSet::RowsAffected() const { return pimpl_->RowsAffected(); }

std::string ResultSet::CommandStatus() const { return pimpl_->CommandStatus(); }

ResultSet::const_iterator ResultSet::cbegin() const& { return {pimpl_, 0}; }

ResultSet::const_iterator ResultSet::cend() const& { return {pimpl_, Size()}; }

ResultSet::const_reverse_iterator ResultSet::crbegin() const& { return {pimpl_, Size() - 1}; }

ResultSet::const_reverse_iterator ResultSet::crend() const& { return {pimpl_, npos}; }

ResultSet::reference ResultSet::Front() const& { return (*this)[0]; }

ResultSet::reference ResultSet::Back() const& { return (*this)[Size() - 1]; }

ResultSet::reference ResultSet::operator[](size_type index) const& {
    if (index >= Size()) throw RowIndexOutOfBounds{index};
    return {pimpl_, index};
}

void ResultSet::FillBufferCategories(const UserTypes& types) { pimpl_->FillBufferCategories(types); }

void ResultSet::SetBufferCategoriesFrom(const ResultSet& dsc) { pimpl_->SetTypeBufferCategories(*dsc.pimpl_); }

}  // namespace storages::postgres

USERVER_NAMESPACE_END
