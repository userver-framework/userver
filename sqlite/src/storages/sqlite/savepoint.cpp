#include <userver/storages/sqlite/savepoint.hpp>

#include <userver/logging/log.hpp>

#include <userver/storages/sqlite/impl/connection.hpp>
#include <userver/storages/sqlite/infra/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

namespace {

constexpr std::string_view kStatementPrepeareString = "SELECT quote(?)";

}  // namespace

std::string Savepoint::PrepareString(const std::string& str) {
    auto params_binder = impl::BindHelper::UpdateParamsBindings(kStatementPrepeareString.data(), *connection_, str);
    return DoExecute(params_binder).AsSingleField<std::string>();
}

Savepoint::Savepoint(std::shared_ptr<infra::ConnectionPtr> connection, std::string name)
    : connection_{std::move(connection)} {
    if (connection_ && connection_->IsValid()) {
        name_ = PrepareString(name);
        (*connection_)->Save(name_);
    }
}

Savepoint::Savepoint(Savepoint&& other) noexcept = default;

Savepoint& Savepoint::operator=(Savepoint&&) noexcept = default;

Savepoint::~Savepoint() {
    if (connection_ && connection_->IsValid()) {
        LOG_INFO() << "Savepoint handle is destroyed without an explicit "
                      "release or rollback_to, rolling back automatically";
        try {
            RollbackTo();
            Release();
        } catch (const std::exception& err) {
            LOG_ERROR() << "Failed to auto rollback a savepoint: " << err;
        }
    }
}

void Savepoint::AssertValid() const {
    // TODO: exception or abort?
    UINVARIANT(connection_ && connection_->IsValid(), "Savepoint accessed after it's been released");
}

void Savepoint::Release() {
    AssertValid();
    {
        auto connection = std::move(connection_);
        (*connection)->Release(name_);
    }
}

void Savepoint::RollbackTo() {
    AssertValid();
    (*connection_)->RollbackTo(name_);
}

Savepoint Savepoint::Save(std::string name) const {
    AssertValid();
    return Savepoint{connection_, std::move(name)};
}

ResultSet Savepoint::DoExecute(impl::io::ParamsBinderBase& params) const {
    auto prepare_statement = params.GetBindsPtr();
    auto result_wrapper = std::make_unique<impl::ResultWrapper>(prepare_statement, connection_);
    return ResultSet{std::move(result_wrapper)};
}

void Savepoint::AccountQueryExecute() const noexcept { (*connection_)->AccountQueryExecute(); }

void Savepoint::AccountQueryFailed() const noexcept { (*connection_)->AccountQueryFailed(); }

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
