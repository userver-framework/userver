#pragma once

#include <memory>

#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/impl/extractor_base.hpp>
#include <userver/storages/sqlite/row_types.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

/// @brief Result fetch helper
class ResultWrapper final {
public:
    ResultWrapper(StatementBasePtr prepare_statement, std::shared_ptr<infra::ConnectionPtr> connection_ptr);
    ~ResultWrapper();

    ResultWrapper(const ResultWrapper&) = delete;
    ResultWrapper(ResultWrapper&&) = delete;

    StatementBasePtr GetStatement() noexcept;

    void FetchAllResult(impl::ExtractorBase& extractor);

    bool FetchResult(impl::ExtractorBase& extractor, size_t batch_size);

    ExecutionResult GetExecutionResult() noexcept;

private:
    void ExecutionStep();

    void AccountQueryCompleted() noexcept;
    void AccountQueryFailed() noexcept;

    StatementBasePtr prepare_statement_;
    std::shared_ptr<infra::ConnectionPtr> connection_ptr_;
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
