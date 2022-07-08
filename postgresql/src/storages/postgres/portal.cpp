#include <userver/storages/postgres/portal.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <userver/storages/postgres/detail/time_types.hpp>
#include <userver/storages/postgres/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

struct Portal::Impl {
  detail::Connection* conn_{nullptr};
  OptionalCommandControl cmd_ctl_;
  detail::Connection::StatementId statement_id_;
  PortalName name_;
  std::size_t fetched_so_far_{0};
  bool done_{false};

  Impl(detail::Connection* conn, const PortalName& name, const Query& query,
       const detail::QueryParameters& params, OptionalCommandControl cmd_ctl)
      : conn_{conn}, cmd_ctl_{std::move(cmd_ctl)}, name_{name} {
    if (conn_) {
      if (!cmd_ctl_) {
        cmd_ctl_ = conn_->GetQueryCmdCtl(query.GetName());
      }
      Bind(query.Statement(), params);
    }
  }

  Impl(Impl&& rhs) noexcept = default;
  Impl& operator=(Impl&& rhs) noexcept {
    Impl{std::move(rhs)}.Swap(*this);
    return *this;
  }

  void Swap(Impl& rhs) noexcept {
    using std::swap;
    swap(conn_, rhs.conn_);
    swap(cmd_ctl_, rhs.cmd_ctl_);
    swap(statement_id_, rhs.statement_id_);
    swap(name_, rhs.name_);
    swap(fetched_so_far_, rhs.fetched_so_far_);
    swap(done_, rhs.done_);
  }

  void Bind(const std::string& statement,
            const detail::QueryParameters& params) {
    statement_id_ =
        conn_->PortalBind(statement, name_.GetUnderlying(), params, cmd_ctl_);
  }

  ResultSet Fetch(std::uint32_t n_rows) {
    if (!done_) {
      UASSERT(conn_);
      auto res = conn_->PortalExecute(statement_id_, name_.GetUnderlying(),
                                      n_rows, cmd_ctl_);
      auto fetched = res.Size();
      // TODO: check command completion in result TAXICOMMON-4505
      if (!n_rows || fetched != n_rows) {
        done_ = true;
      }
      fetched_so_far_ += fetched;
      return res;
    } else {
      // TODO Specific exception
      throw RuntimeError{"Portal is done, no more data to fetch"};
    }
  }
};

Portal::Portal(detail::Connection* conn, const Query& query,
               const detail::QueryParameters& params,
               OptionalCommandControl cmd_ctl)
    : pimpl_(conn, PortalName{}, query, params, std::move(cmd_ctl)) {}

Portal::Portal(detail::Connection* conn, const PortalName& name,
               const Query& query, const detail::QueryParameters& params,
               OptionalCommandControl cmd_ctl)
    : pimpl_(conn, name, query, params, std::move(cmd_ctl)) {}

Portal::Portal(Portal&&) noexcept = default;
Portal::~Portal() = default;

Portal& Portal::operator=(Portal&&) noexcept = default;

ResultSet Portal::Fetch(std::uint32_t n_rows) { return pimpl_->Fetch(n_rows); }

bool Portal::Done() const { return pimpl_->done_; }
std::size_t Portal::FetchedSoFar() const { return pimpl_->fetched_so_far_; }

}  // namespace storages::postgres

USERVER_NAMESPACE_END
