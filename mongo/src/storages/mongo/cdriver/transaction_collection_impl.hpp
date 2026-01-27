#pragma once

#include <mongoc/mongoc.h>

#include <storages/mongo/cdriver/collection_impl.hpp>
#include <storages/mongo/cdriver/wrappers.hpp>
#include <storages/mongo/transaction_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

/// @brief MongoDB collection implementation that operates within a transaction
class CDriverTransactionCollectionImpl : public CollectionImpl {
public:
    CDriverTransactionCollectionImpl(
        PoolImplPtr pool_impl,
        std::string database_name,
        std::string collection_name,
        std::shared_ptr<TransactionData> data
    );

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

private:
    template <typename Operation>
    auto DoExecute(Operation&& op);

    template <typename Operation>
    auto DoExecute(Operation&& op) const;

    template <typename Operation>
    RequestContext MakeTransactionRequestContext(std::string&& span_name, const Operation& operation) const;

    FindAndModifyOptsPtr CopyFAMOptsAndSetSession(const FindAndModifyOptsPtr& old_options) const;

    CDriverCollectionImpl collection_;
    std::shared_ptr<TransactionData> data_;
};

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
