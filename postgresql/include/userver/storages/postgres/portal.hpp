#pragma once

#include <memory>
#include <string>

#include <engine/deadline.hpp>

#include <storages/postgres/detail/connection_ptr.hpp>
#include <storages/postgres/detail/query_parameters.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/postgres_fwd.hpp>
#include <storages/postgres/query.hpp>
#include <storages/postgres/result_set.hpp>

#include <utils/fast_pimpl.hpp>
#include <utils/strong_typedef.hpp>

namespace storages::postgres {

using PortalName = ::utils::StrongTypedef<struct PortalNameTag, std::string>;

class Portal {
 public:
  Portal(detail::Connection* conn, const Query& query,
         const detail::QueryParameters& = {},
         OptionalCommandControl cmd_ctl = {});
  Portal(detail::Connection* conn, const PortalName&, const Query& query,
         const detail::QueryParameters& = {},
         OptionalCommandControl cmd_ctl = {});

  Portal(Portal&&) noexcept;
  Portal& operator=(Portal&&) noexcept;

  Portal(const Portal&) = delete;
  Portal& operator=(const Portal&) = delete;

  ~Portal();

  ResultSet Fetch(std::uint32_t n_rows);

  bool Done() const;
  std::size_t FetchedSoFar() const;

  explicit operator bool() const { return !Done(); }

 private:
  static constexpr std::size_t kImplSize = 88;
  static constexpr std::size_t kImplAlign = 8;

  struct Impl;
  ::utils::FastPimpl<Impl, kImplSize, kImplAlign> pimpl_;
};

}  // namespace storages::postgres
