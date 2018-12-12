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

std::string FirstHostAndPortFromDsn(const std::string& conninfo);
std::string FirstHostNameFromDsn(const std::string& conninfo);
std::string FirstDbNameFromDsn(const std::string& conninfo);

std::string EscapeHostName(const std::string& hostname, char escape_char = '_');

}  // namespace postgres
}  // namespace storages
