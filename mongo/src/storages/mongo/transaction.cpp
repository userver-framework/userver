#include <userver/storages/mongo/transaction.hpp>

#include <storages/mongo/transaction_impl.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

Transaction::Transaction() = default;

Transaction::Transaction(std::unique_ptr<impl::TransactionImpl> impl)
    : impl_(std::move(impl))
{}

Transaction::Transaction(Transaction&&) noexcept = default;

Transaction& Transaction::operator=(Transaction&&) noexcept = default;

Transaction::~Transaction() {
    if (impl_ && IsActive()) {
        impl_->Abort();
    }
}

Collection Transaction::GetCollection(std::string name) {
    if (!impl_) {
        throw MongoException("Transaction is not initialized");
    }
    return impl_->GetCollection(std::move(name));
}

void Transaction::Commit() {
    if (!impl_) {
        throw MongoException("Transaction is not initialized");
    }
    impl_->Commit();
}

void Transaction::Abort() noexcept {
    if (impl_) {
        impl_->Abort();
    }
}

bool Transaction::IsActive() const noexcept { return impl_ && impl_->IsActive(); }

Transaction::State Transaction::GetState() const noexcept {
    if (!impl_) {
        return State::kNone;
    }
    return impl_->GetState();
}

}  // namespace storages::mongo

USERVER_NAMESPACE_END
