#include <storages/mongo/cdriver/transaction_collection_impl.hpp>

#include <formats/bson/wrappers.hpp>
#include <storages/mongo/cdriver/find_and_modify.hpp>
#include <storages/mongo/operations_common.hpp>
#include <storages/mongo/operations_impl.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/operations.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

namespace {

void AppendSessionId(const TransactionData& data, bson_t* opts) {
    data.EnsureActive();

    MongoError error;
    if (!mongoc_client_session_append(data.session.get(), opts, error.GetNative())) {
        error.Throw("Error appending session to opts");
    }
}

}  // namespace

CDriverTransactionCollectionImpl::CDriverTransactionCollectionImpl(
    PoolImplPtr pool_impl,
    std::string database_name,
    std::string collection_name,
    std::shared_ptr<TransactionData> data
)
    : CDriverCollectionImpl(std::move(pool_impl), std::move(database_name), std::move(collection_name)),
      data_(std::move(data))
{
    UASSERT(data_);
    UASSERT(data_->session);
    UASSERT(data_->client);
}

CDriverPoolImpl::BoundClientPtr CDriverTransactionCollectionImpl::GetClient(stats::OperationStatisticsItem& /*stats*/
) const {
    UASSERT(data_->client);
    return CDriverPoolImpl::BoundClientPtr::Borrowed(data_->client.value());
}

template <typename Operation>
auto CDriverTransactionCollectionImpl::DoExecute(Operation&& op) {
    auto operation = std::forward<Operation>(op);
    AppendSessionId(*data_, EnsureBuilder(operation.impl_->options).Get());

    return CDriverCollectionImpl::Execute(operation);
}

FindAndModifyOptsPtr CDriverTransactionCollectionImpl::CopyFAMOptsAndSetSession(const FindAndModifyOptsPtr& old_options
) const {
    auto options = CopyFindAndModifyOptions(old_options);
    formats::bson::impl::MutableBson opts;
    AppendSessionId(*data_, opts.Get());
    if (!mongoc_find_and_modify_opts_append(options.get(), opts.Get())) {
        throw MongoException("Cannot set 'extra'");
    }
    return options;
}

template <>
auto CDriverTransactionCollectionImpl::DoExecute(const operations::FindAndModify& op)
{
    auto options = CopyFAMOptsAndSetSession(op.impl_->options);
    operations::FindAndModify operation(op.impl_->CloneWithOptions(std::move(options)));

    return CDriverCollectionImpl::Execute(operation);
}

template <>
auto CDriverTransactionCollectionImpl::DoExecute(operations::Bulk&& op)
{
    data_->EnsureActive();
    mongoc_bulk_operation_set_client_session(op.impl_->bulk.get(), data_->session.get());
    return CDriverCollectionImpl::Execute(std::move(op));
}

template <>
auto CDriverTransactionCollectionImpl::DoExecute(const operations::FindAndRemove& op)
{
    auto options = CopyFAMOptsAndSetSession(op.impl_->options);
    operations::FindAndRemove operation(op.impl_->CloneWithOptions(std::move(options)));

    return CDriverCollectionImpl::Execute(operation);
}

template <typename Operation>
auto CDriverTransactionCollectionImpl::DoExecute(Operation&& op) const {
    auto operation = std::forward<Operation>(op);
    AppendSessionId(*data_, EnsureBuilder(operation.impl_->options).Get());

    return CDriverCollectionImpl::Execute(operation);
}

size_t CDriverTransactionCollectionImpl::Execute(const operations::Count& count_op) const {
    return DoExecute(count_op);
}

size_t CDriverTransactionCollectionImpl::Execute(const operations::CountApprox& count_op) const {
    return DoExecute(count_op);
}

Cursor CDriverTransactionCollectionImpl::Execute(const operations::Find& find_op) const { return DoExecute(find_op); }

std::vector<formats::bson::Value> CDriverTransactionCollectionImpl::Execute(const operations::Distinct& distinct_op
) const {
    return DoExecute(distinct_op);
}

WriteResult CDriverTransactionCollectionImpl::Execute(const operations::InsertOne& insert_op) {
    return DoExecute(insert_op);
}

WriteResult CDriverTransactionCollectionImpl::Execute(const operations::InsertMany& insert_op) {
    return DoExecute(insert_op);
}

WriteResult CDriverTransactionCollectionImpl::Execute(const operations::ReplaceOne& replace_op) {
    return DoExecute(replace_op);
}

WriteResult CDriverTransactionCollectionImpl::Execute(const operations::Update& update_op) {
    return DoExecute(update_op);
}

WriteResult CDriverTransactionCollectionImpl::Execute(const operations::Delete& delete_op) {
    return DoExecute(delete_op);
}

WriteResult CDriverTransactionCollectionImpl::Execute(const operations::FindAndModify& fam_op) {
    return DoExecute(fam_op);
}

WriteResult CDriverTransactionCollectionImpl::Execute(const operations::FindAndRemove& far_op) {
    return DoExecute(far_op);
}

WriteResult CDriverTransactionCollectionImpl::Execute(operations::Bulk&& bulk_op) {
    return DoExecute(std::move(bulk_op));
}

Cursor CDriverTransactionCollectionImpl::Execute(const operations::Aggregate& aggregate_op) {
    return DoExecute(aggregate_op);
}

void CDriverTransactionCollectionImpl::Execute(const operations::Drop& drop_op) { DoExecute(drop_op); }

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
