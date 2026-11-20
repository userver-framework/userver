#include <storages/mongo/transaction_impl.hpp>

#include <mongoc/mongoc.h>

#include <formats/bson/wrappers.hpp>
#include <storages/mongo/cdriver/pool_impl.hpp>
#include <storages/mongo/cdriver/transaction_collection_impl.hpp>
#include <storages/mongo/cdriver/wrappers.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/mongo_error.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

namespace {

TransactionException::Type GetCommitErrorType(const bson_t* reply) {
    if (mongoc_error_has_label(reply, "UnknownTransactionCommitResult")) {
        return TransactionException::Type::kUnknownCommitResult;
    }
    if (mongoc_error_has_label(reply, "TransientTransactionError")) {
        return TransactionException::Type::kTransientError;
    }
    return TransactionException::Type::kPermanentError;
}

}  // namespace

void TransactionData::EnsureActive() const {
    if (state == Transaction::State::kNone) {
        throw MongoException("Transaction not started. Call a transaction method first.");
    } else if (state != Transaction::State::kInProgress) {
        throw MongoException("Transaction is not active");
    }
}

void TransactionData::ReleaseClient() {
    session.reset();
    client.reset();
}

TransactionImpl::TransactionImpl(std::shared_ptr<PoolImpl> pool_impl)
    : pool_impl_(std::move(pool_impl)),
      data_{std::make_shared<TransactionData>(Transaction::State::kNone, nullptr)}
{
    UASSERT(pool_impl_);
}

TransactionImpl::~TransactionImpl() {
    if (IsActive()) {
        LOG_INFO()
            << "Transaction handle is destroyed without an explicit "
               "commit or rollback, rolling back automatically";
        Abort();
    }
    data_->ReleaseClient();
}

Collection TransactionImpl::GetCollection(std::string name) {
    // Start transaction lazily if not started yet
    if (data_->state == Transaction::State::kNone) {
        EnsureTransactionStarted();
    }
    data_->EnsureActive();

    auto collection_impl = std::make_shared<
        cdriver::CDriverTransactionCollectionImpl>(pool_impl_, std::move(name), data_);
    return Collection{std::move(collection_impl), /*transactional=*/true};
}

void TransactionImpl::Commit() {
    if (data_->state != Transaction::State::kCommitUnknown) {
        data_->EnsureActive();
    }
    if (!data_->session) {
        throw MongoException("Cannot retry commit after the transaction session has been released");
    }

    MongoError error;
    formats::bson::impl::UninitializedBson reply;

    if (!mongoc_client_session_commit_transaction(data_->session.get(), reply.Get(), error.GetNative())) {
        const auto error_type = GetCommitErrorType(reply.Get());
        if (error_type == TransactionException::Type::kUnknownCommitResult) {
            data_->state = Transaction::State::kCommitUnknown;
            throw TransactionException(
                error_type,
                "Transaction commit result is unknown: " + std::string(error.Message())
            );
        }
        data_->state = Transaction::State::kAborted;
        data_->ReleaseClient();
        throw TransactionException(error_type, "Transaction commit failed: " + std::string(error.Message()));
    }

    data_->state = Transaction::State::kCommitted;
    data_->ReleaseClient();

    LOG_DEBUG() << "Transaction committed successfully";
}

void TransactionImpl::Abort() noexcept {
    if (data_->state == Transaction::State::kCommitUnknown) {
        data_->ReleaseClient();
        return;
    }
    if (!IsActive()) {
        return;
    }
    utils::FastScopeGuard release([&]() noexcept { data_->ReleaseClient(); });

    data_->state = Transaction::State::kAborted;

    MongoError error;
    if (!mongoc_client_session_abort_transaction(data_->session.get(), error.GetNative())) {
        LOG_WARNING() << "Failed to abort transaction: " << error.Message();
    }

    LOG_DEBUG() << "Transaction aborted";
}

bool TransactionImpl::IsActive() const noexcept { return data_->state == Transaction::State::kInProgress; }

Transaction::State TransactionImpl::GetState() const noexcept { return data_->state; }

void TransactionImpl::EnsureTransactionStarted() {
    if (data_->state != Transaction::State::kNone) {
        return;
    }

    // Cast to CDriverPoolImpl to access Acquire method
    UASSERT(dynamic_cast<cdriver::CDriverPoolImpl*>(pool_impl_.get()));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto* cdriver_pool = static_cast<cdriver::CDriverPoolImpl*>(pool_impl_.get());

    // Acquire a client and keep it alive for the entire transaction: the
    // mongoc_client_session_t created below stores an internal raw pointer to
    // this client, so it must not be returned to the pool until the session is
    // destroyed.
    data_->client.emplace(cdriver_pool->Acquire());

    // Create session
    MongoError error;
    data_->session = cdriver::SessionPtr(
        mongoc_client_start_session(data_->client->get(), nullptr, error.GetNative()),
        cdriver::SessionDeleter()
    );
    if (!data_->session) {
        throw MongoException("Failed to start session: " + std::string(error.Message()));
    }

    // Start transaction
    if (!mongoc_client_session_start_transaction(data_->session.get(), nullptr, error.GetNative())) {
        throw TransactionException(
            TransactionException::Type::kTransientError,
            "Failed to start transaction: " + std::string(error.Message())
        );
    }

    data_->state = Transaction::State::kInProgress;

    LOG_DEBUG() << "Transaction started successfully";
}

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
