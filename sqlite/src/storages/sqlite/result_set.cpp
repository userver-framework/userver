#include <userver/storages/sqlite/result_set.hpp>

#include <userver/storages/sqlite/impl/result_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

ResultSet::ResultSet(impl::ResultWrapperPtr pimpl) : pimpl_{std::move(pimpl)} {}

ResultSet::ResultSet(ResultSet&& other) noexcept = default;

ResultSet& ResultSet::operator=(ResultSet&&) noexcept = default;

ResultSet::~ResultSet() = default;

ExecutionResult ResultSet::AsExecutionResult() && {
  return pimpl_->GetExecutionResult();
}

void ResultSet::FetchResult(impl::ExtractorBase& extractor) {
  pimpl_->FetchResult(extractor);
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
