#pragma once

#include <memory>

#include <storages/mongo/cdriver/request_helpers.hpp>
#include <storages/mongo/cdriver/wrappers.hpp>
#include <storages/mongo/collection_impl.hpp>
#include <storages/mongo/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

struct CollectionRequestContext : RequestContextBase {
    CollectionPtr collection;
};

class CDriverCollectionImpl : public CollectionImpl {
public:
    CDriverCollectionImpl(PoolImplPtr pool_impl, std::string database_name, std::string collection_name);

    size_t Execute(const operations::Count&) const override;
    size_t Execute(const operations::CountApprox&) const override;
    Cursor Execute(const operations::Find&) const override;
    std::vector<formats::bson::Value> Execute(const operations::Distinct&) const override;
    WriteResult Execute(const operations::InsertOne&) override;
    WriteResult Execute(const operations::InsertMany&) override;
    WriteResult Execute(const operations::ReplaceOne&) override;
    WriteResult Execute(const operations::Update&) override;
    WriteResult Execute(const operations::Delete&) override;
    WriteResult Execute(const operations::FindAndModify&) override;
    WriteResult Execute(const operations::FindAndRemove&) override;
    WriteResult Execute(operations::Bulk&&) override;
    Cursor Execute(const operations::Aggregate&) override;
    void Execute(const operations::Drop&) override;

protected:
    virtual cdriver::CDriverPoolImpl::BoundClientPtr GetClient(stats::OperationStatisticsItem& stats) const;

    [[maybe_unused]] virtual mongoc_client_session_t* GetSession() const;
    ReadPrefsPtr MakeEffectiveReadPrefs(const ReadPrefsPtr& operation_read_prefs) const;

private:
    CollectionRequestContext MakeRequestContext(std::string&& span_name, const stats::OperationKey& stats_key) const;

    template <typename Operation>
    CollectionRequestContext MakeRequestContext(std::string&& span_name, const Operation& operation) const;

    WriteResult ExecuteReplaceNative(const operations::ReplaceOne& operation, CollectionRequestContext& context);
    WriteResult ExecuteUpdateNative(const operations::Update& operation, CollectionRequestContext& context);

    cdriver::CDriverPoolImpl& GetPool() const;

    PoolImplPtr pool_impl_;
    std::shared_ptr<stats::CollectionStatistics> statistics_;
};

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
