#pragma once

#include <string>
#include <vector>

namespace storages {
namespace postgres {

using DSNList = std::vector<std::string>;
DSNList SplitByHost(const std::string& conninfo);

/// Create a string <host>_<port>_<dbname>_<user> of a single-host DSN
/// For testing purposes
std::string MakeDsnNick(const std::string& conninfo);

}  // namespace postgres
}  // namespace storages
