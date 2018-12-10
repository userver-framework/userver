#include <storages/postgres/dsn.hpp>

#include <storages/postgres/exceptions.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>

#include <boost/algorithm/string/split.hpp>

#include <libpq-fe.h>

namespace storages {
namespace postgres {

namespace {

const std::string kPostgreSQLDefaultHost = "localhost";
const std::string kPostgreSQLDefaultPort = "5432";

std::vector<std::string> SplitString(const char* str, char delim = ',') {
  std::vector<std::string> res;
  boost::split(res, str, [delim](char c) { return c == delim; });
  return res;
}

using OptionsHandle =
    std::unique_ptr<PQconninfoOption, decltype(&PQconninfoFree)>;

OptionsHandle MakeDSNOptions(const std::string& conninfo) {
  char* errmsg = nullptr;
  OptionsHandle opts{PQconninfoParse(conninfo.c_str(), &errmsg),
                     &PQconninfoFree};

  if (errmsg) {
    InvalidDSN err{conninfo, errmsg};
    PQfreemem(errmsg);
    throw err;
  }
  return opts;
}

struct HostAndPort {
  using StringList = std::vector<std::string>;

  StringList hosts;
  StringList ports;
};

HostAndPort ParseDSNOptions(
    const std::string& conninfo,
    std::function<void(PQconninfoOption*)> other_opt_builder =
        [](PQconninfoOption*) {}) {
  HostAndPort hap;
  auto opts = MakeDSNOptions(conninfo);

  auto opt = opts.get();
  while (opt != nullptr && opt->keyword != nullptr) {
    if (opt->val) {
      if (::strcmp(opt->keyword, "host") == 0) {
        hap.hosts = SplitString(opt->val);
      } else if (::strcmp(opt->keyword, "port") == 0) {
        hap.ports = SplitString(opt->val);
      } else {
        other_opt_builder(opt);
      }
    }
    ++opt;
  }
  return hap;
}

}  // namespace

DSNList SplitByHost(const std::string& conninfo) {
  std::ostringstream options;
  const auto hap = ParseDSNOptions(conninfo, [&options](PQconninfoOption* opt) {
    options << " " << opt->keyword << "=" << opt->val;
  });

  const auto& hosts = hap.hosts;
  const auto& ports = hap.ports;
  DSNList res;
  if (hosts.empty()) {
    // default host, just return the options string
    if (ports.size() > 1)
      throw InvalidDSN{conninfo, "Invalid port options count"};
    if (!ports.empty()) {
      options << " "
              << "port=" << ports.front();
    }
    res.push_back(options.str());
  } else {
    if (ports.size() > 1 && ports.size() != hosts.size()) {
      throw InvalidDSN{conninfo, "Invalid port options count"};
    }
    for (auto host = hosts.begin(); host != hosts.end(); ++host) {
      std::ostringstream os;
      os << "host=" << *host;
      if (ports.size() == 1) {
        os << " port=" << ports.front();
      } else if (!ports.empty()) {
        os << " port=" << ports[host - hosts.begin()];
      }
      os << options.str();
      res.push_back(os.str());
    }
  }

  return res;
}

std::string MakeDsnNick(const std::string& conninfo, bool escape) {
  std::map<std::string, std::string> opt_dict{{"host", kPostgreSQLDefaultHost},
                                              {"port", kPostgreSQLDefaultPort}};

  const auto hap =
      ParseDSNOptions(conninfo, [&opt_dict](PQconninfoOption* opt) {
        opt_dict[opt->keyword] = opt->val;
      });

  const auto& hosts = hap.hosts;
  if (!hosts.empty()) opt_dict["host"] = hosts.front();

  const auto& ports = hap.ports;
  if (!ports.empty()) opt_dict["port"] = ports.front();

  std::string dsn_str;
  std::array<const char*, 4> keys = {"user", "host", "port", "dbname"};
  std::array<char, 3> delims = {'@', ':', '/'};
  for (size_t i = 0; i < keys.size(); ++i) {
    auto const& key = keys[i];
    if (opt_dict[key].empty()) {
      continue;
    }

    if (i != 0) {
      dsn_str += escape ? '_' : delims[i - 1];
    }
    dsn_str += opt_dict[key];
  }

  if (escape) {
    dsn_str.erase(std::remove_if(dsn_str.begin(), dsn_str.end(),
                                 [](char c) {
                                   return !std::isalpha(c) &&
                                          !std::isdigit(c) && c != '_';
                                 }),
                  dsn_str.end());
  }
  return dsn_str;
}

std::string FirstHostAndPortFromDsn(const std::string& conninfo) {
  const auto hap = ParseDSNOptions(conninfo);
  const auto host =
      hap.hosts.empty() ? kPostgreSQLDefaultHost : hap.hosts.front();
  const auto port =
      hap.ports.empty() ? kPostgreSQLDefaultPort : hap.ports.front();
  return host + ':' + port;
}

std::string FirstHostNameFromDsn(const std::string& conninfo) {
  const auto hap = ParseDSNOptions(conninfo);
  auto host = hap.hosts.empty() ? kPostgreSQLDefaultHost : hap.hosts.front();
  return host;
}

std::string FirstDbNameFromDsn(const std::string& conninfo) {
  std::string db_name;
  ParseDSNOptions(conninfo, [&db_name](PQconninfoOption* opt) {
    if (db_name.empty() && opt->keyword == std::string("dbname")) {
      db_name = opt->val;
    }
  });
  return db_name;
}

std::string EscapeHostName(const std::string& hostname, char escape_char) {
  auto escaped = hostname;
  std::replace_if(escaped.begin(), escaped.end(),
                  [](char c) { return !std::isalpha(c) && !std::isdigit(c); },
                  escape_char);
  return escaped;
}

}  // namespace postgres
}  // namespace storages
