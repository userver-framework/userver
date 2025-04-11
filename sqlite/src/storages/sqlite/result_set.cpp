#include <userver/storages/sqlite/result_set.hpp>

#include <userver/storages/sqlite/impl/result_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

ResultSet::ResultSet(impl::ResultWrapperPtr pimpl) : pimpl_{std::move(pimpl)} {}

ResultSet::ResultSet(ResultSet&& other) noexcept = default;

ResultSet& ResultSet::operator=(ResultSet&&) noexcept = default;

ResultSet::~ResultSet() = default;

ExecutionResult ResultSet::AsExecutionResult() && { return pimpl_->GetExecutionResult(); }

void ResultSet::FetchAllResult(impl::ExtractorBase& extractor) { pimpl_->FetchAllResult(extractor); }

bool ResultSet::FetchResult(impl::ExtractorBase& extractor, size_t batch_size) {
    return pimpl_->FetchResult(extractor, batch_size);
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
