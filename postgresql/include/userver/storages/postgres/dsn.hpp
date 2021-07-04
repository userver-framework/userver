#pragma once

/// @file userver/storages/postgres/dsn.hpp
/// @brief DSN manipulation helpers

#include <string>
#include <vector>

#include <userver/utils/strong_typedef.hpp>

namespace storages::postgres {

using Dsn = ::utils::NonLoggable<class DsnTag, std::string>;
using DsnList = std::vector<Dsn>;

DsnList SplitByHost(const Dsn& dsn);

/// Create a string <user>@<host>:<port>/<dbname> of a single-host DSN
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

/// Return <host>[:<port>] string for a single-host DSN
std::string GetHostPort(const Dsn& dsn);

/// Return DSN string without 'password' field
std::string DsnCutPassword(const Dsn& dsn);
/// Return DSN string with password contents masked
std::string DsnMaskPassword(const Dsn& dsn);

std::string EscapeHostName(const std::string& hostname, char escape_char = '_');

}  // namespace storages::postgres
