#pragma once

/// @file userver/storages/postgres/dsn.hpp
/// @brief DSN manipulation helpers

#include <string>
#include <vector>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

using Dsn = USERVER_NAMESPACE::utils::NonLoggable<class DsnTag, std::string>;
using DsnList = std::vector<Dsn>;

DsnList SplitByHost(const Dsn& dsn);

/// Create a string `<user>@<host>:<port>/<dbname>` of a single-host DSN
/// or escape all the punctuation with _ for test
std::string MakeDsnNick(const Dsn& dsn, bool escape);

struct DsnOptions {
  std::string host;
  std::string port;
  std::string dbname;
};

/// Read options from a DSN
/// First unique option is used if found, otherwise default is taken
DsnOptions OptionsFromDsn(const Dsn& dsn);

/// Return `<host>[:<port>]` string for a single-host DSN
std::string GetHostPort(const Dsn& dsn);

/// Return DSN string without 'password' field
std::string DsnCutPassword(const Dsn& dsn);
/// Return DSN string with password contents masked
std::string DsnMaskPassword(const Dsn& dsn);

std::string EscapeHostName(const std::string& hostname, char escape_char = '_');

/// Return DSN string with hosts resolved as hostaddr values
/// If given DSN has no host or already contains hostaddr, does nothing
Dsn ResolveDsnHostaddrs(const Dsn& dsn, clients::dns::Resolver& resolver,
                        engine::Deadline deadline);

}  // namespace storages::postgres

USERVER_NAMESPACE_END
