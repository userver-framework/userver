#pragma once

#include <memory>
#include <string>

#include <engine/deadline.hpp>

#include <storages/postgres/detail/connection_ptr.hpp>
#include <storages/postgres/detail/query_parameters.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/postgres_fwd.hpp>
#include <storages/postgres/result_set.hpp>

#include <utils/strong_typedef.hpp>

namespace storages::postgres {

using PortalName = ::utils::StrongTypedef<struct PortalNameTag, std::string>;

class Portal {
 public:
  Portal(detail::Connection* conn, const std::string& statement,
         const detail::QueryParameters& = {},
         OptionalCommandControl cmd_ctl = {});
  Portal(detail::Connection* conn, const PortalName&,
         const std::string& statement, const detail::QueryParameters& = {},
         OptionalCommandControl cmd_ctl = {});

  Portal(Portal&&) noexcept;
  // no noexcept here as std::string doesn't provide one
  Portal& operator=(Portal&&);

  Portal(const Portal&) = delete;
  Portal& operator=(const Portal&) = delete;

  ~Portal();

  ResultSet Fetch(std::uint32_t n_rows);

  bool Done() const { return done_; }
  std::size_t FetchedSoFar() const { return fetched_so_far_; }

  explicit operator bool() const { return !done_; }

 private:
  void Bind(const std::string& statement, const detail::QueryParameters&);

 private:
  detail::Connection* conn_{nullptr};
  OptionalCommandControl cmd_ctl_;
  PortalName name_;
  std::size_t fetched_so_far_{0};
  bool done_{false};
};

}  // namespace storages::postgres
