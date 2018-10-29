
#include <storages/postgres/dsn.hpp>

#include <storages/postgres/exceptions.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>

#include <boost/algorithm/string/split.hpp>

#include <postgresql/libpq-fe.h>

namespace storages {
namespace postgres {

namespace {

const std::string kPostgreSQLDefaultPort = "5432";

std::vector<std::string> SplitString(const char* str, char delim = ',') {
  std::vector<std::string> res;
  boost::split(res, str, [delim](char c) { return c == delim; });
  return res;
}

using OptionsHandle =
    std::unique_ptr<PQconninfoOption, decltype(&PQconninfoFree)>;

OptionsHandle ParseDSNOptions(const std::string& conninfo) {
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

}  // namespace

DSNList SplitByHost(const std::string& conninfo) {
  using StringList = std::vector<std::string>;

  auto opts = ParseDSNOptions(conninfo);

  StringList hosts;
  StringList ports;
  std::ostringstream options;

  auto opt = opts.get();
  // TODO Refactor out this cycle
  while (opt != nullptr && opt->keyword != nullptr) {
    if (opt->val) {
      if (::strcmp(opt->keyword, "host") == 0) {
        hosts = SplitString(opt->val);
      } else if (::strcmp(opt->keyword, "port") == 0) {
        ports = SplitString(opt->val);
      } else {
        options << " " << opt->keyword << "=" << opt->val;
      }
    }
    ++opt;
  }

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

std::string MakeDsnNick(const std::string& conninfo) {
  auto opts = ParseDSNOptions(conninfo);
  std::map<std::string, std::string> opt_dict{{"host", "localhost"},
                                              {"port", kPostgreSQLDefaultPort}};
  auto opt = opts.get();
  // TODO Refactor out this cycle
  while (opt != nullptr && opt->keyword != nullptr) {
    if (opt->val) {
      if (::strcmp(opt->keyword, "host") == 0) {
        auto hosts = SplitString(opt->val);
        if (!hosts.empty()) opt_dict["host"] = hosts.front();
      } else if (::strcmp(opt->keyword, "port") == 0) {
        auto ports = SplitString(opt->val);
        if (!ports.empty()) opt_dict["port"] = ports.front();
      } else {
        opt_dict[opt->keyword] = opt->val;
      }
    }
    ++opt;
  }
  std::ostringstream os;
  for (auto const& key : {"host", "port", "dbname", "user"}) {
    if (!opt_dict[key].empty()) {
      if (!os.str().empty()) {
        os << "_";
      }
      os << opt_dict[key];
    }
  }
  auto dsn_str = os.str();
  dsn_str.erase(std::remove_if(dsn_str.begin(), dsn_str.end(),
                               [](char c) {
                                 return !std::isalpha(c) && !std::isdigit(c) &&
                                        c != '_';
                               }),
                dsn_str.end());
  return dsn_str;
}

}  // namespace postgres
}  // namespace storages
