#include <storages/postgres/portal.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/time_types.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages::postgres {

namespace {

const std::string kEmptyPortalName{};

}  // namespace

Portal::Portal(detail::Connection* conn, const std::string& statement,
               const detail::QueryParameters& params,
               OptionalCommandControl cmd_ctl)
    : conn_{conn}, cmd_ctl_{std::move(cmd_ctl)} {
  // Allow nullptr here for tests
  if (conn_) {
    Bind(statement, params);
  }
}

Portal::Portal(detail::Connection* conn, const PortalName& name,
               const std::string& statement,
               const detail::QueryParameters& params,
               OptionalCommandControl cmd_ctl)
    : conn_{conn}, cmd_ctl_{std::move(cmd_ctl)}, name_{name} {
  // Allow nullptr here for tests
  if (conn_) {
    Bind(statement, params);
  }
}

Portal::Portal(Portal&&) noexcept = default;
Portal::~Portal() = default;

// std::string doesn't provide a noexcept move assignment
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
Portal& Portal::operator=(Portal&&) = default;

void Portal::Bind(const std::string& statement,
                  const detail::QueryParameters& params) {
  conn_->PortalBind(statement, name_.GetUnderlying(), params, cmd_ctl_);
}

ResultSet Portal::Fetch(std::uint32_t n_rows) {
  if (!done_) {
    auto res = conn_->PortalExecute(name_.GetUnderlying(), n_rows, cmd_ctl_);
    auto fetched = res.Size();
    if (fetched != n_rows) {
      done_ = true;
    }
    fetched_so_far_ += fetched;
    return res;
  } else {
    // TODO Specific exception
    throw RuntimeError{"Portal is done, no more data to fetch"};
  }
}

}  // namespace storages::postgres
