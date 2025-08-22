#include <userver/storages/postgres/portal.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <userver/storages/postgres/detail/time_types.hpp>
#include <userver/storages/postgres/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

struct Portal::Impl {
    detail::Connection* conn{nullptr};
    OptionalCommandControl cmd_ctl;
    detail::Connection::StatementId statement_id;
    PortalName name;
    std::size_t fetched_so_far{0};
    bool done{false};

    Impl(
        detail::Connection* conn,
        const PortalName& name,
        const Query& query,
        const detail::QueryParameters& params,
        OptionalCommandControl cmd_ctl
    )
        : conn{conn}, cmd_ctl{std::move(cmd_ctl)}, name{name} {
        if (conn) {
            if (!cmd_ctl) {
                cmd_ctl = conn->GetQueryCmdCtl(query.GetOptionalNameView());
            }
            Bind(query.GetStatementView(), params);
        }
    }

    Impl(Impl&& rhs) noexcept = default;
    Impl& operator=(Impl&& rhs) noexcept {
        Impl{std::move(rhs)}.Swap(*this);
        return *this;
    }

    void Swap(Impl& rhs) noexcept {
        using std::swap;
        swap(conn, rhs.conn);
        swap(cmd_ctl, rhs.cmd_ctl);
        swap(statement_id, rhs.statement_id);
        swap(name, rhs.name);
        swap(fetched_so_far, rhs.fetched_so_far);
        swap(done, rhs.done);
    }

    void Bind(USERVER_NAMESPACE::utils::zstring_view statement, const detail::QueryParameters& params) {
        statement_id = conn->PortalBind(statement, name.GetUnderlying(), params, cmd_ctl);
    }

    ResultSet Fetch(std::uint32_t n_rows) {
        if (!done) {
            UASSERT(conn);
            auto res = conn->PortalExecute(statement_id, name.GetUnderlying(), n_rows, cmd_ctl);
            auto fetched = res.Size();
            // TODO: check command completion in result TAXICOMMON-4505
            if (!n_rows || fetched != n_rows) {
                done = true;
            }
            fetched_so_far += fetched;
            return res;
        } else {
            // TODO Specific exception
            throw RuntimeError{"Portal is done, no more data to fetch"};
        }
    }
};

Portal::Portal(
    detail::Connection* conn,
    const Query& query,
    const detail::QueryParameters& params,
    OptionalCommandControl cmd_ctl
)
    : pimpl_(conn, PortalName{}, query, params, std::move(cmd_ctl)) {}

Portal::Portal(
    detail::Connection* conn,
    const PortalName& name,
    const Query& query,
    const detail::QueryParameters& params,
    OptionalCommandControl cmd_ctl
)
    : pimpl_(conn, name, query, params, std::move(cmd_ctl)) {}

Portal::Portal(Portal&&) noexcept = default;
Portal::~Portal() = default;

Portal& Portal::operator=(Portal&&) noexcept = default;

ResultSet Portal::Fetch(std::uint32_t n_rows) { return pimpl_->Fetch(n_rows); }

bool Portal::Done() const { return pimpl_->done; }
std::size_t Portal::FetchedSoFar() const { return pimpl_->fetched_so_far; }

bool Portal::IsSupportedByDriver() noexcept {
#ifndef USERVER_NO_LIBPQ_PATCHES
    return true;
#else
    return false;
#endif
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
