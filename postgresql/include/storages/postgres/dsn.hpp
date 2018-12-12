#pragma once

#include <string>
#include <vector>

namespace storages {
namespace postgres {

using DSNList = std::vector<std::string>;
DSNList SplitByHost(const std::string& conninfo);

/// Create a string <user>@<host>:<port>/<dbname> of a single-host DSN
/// or escape all the punctuation with _ for test
std::string MakeDsnNick(const std::string& conninfo, bool escape);

struct DsnOptions {
  std::string host;
  std::string port;
  std::string dbname;
};

/// Read options from a DSN
/// First unique option is used if found, otherwise default is taken
DsnOptions OptionsFromDsn(const std::string& conninfo);

/// Return DSN string without 'password' field
std::string DsnCutPassword(const std::string& conninfo);

std::string EscapeHostName(const std::string& hostname, char escape_char = '_');

}  // namespace postgres
}  // namespace storages
