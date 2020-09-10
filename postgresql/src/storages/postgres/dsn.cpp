#include <storages/postgres/dsn.hpp>

#include <storages/postgres/exceptions.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/regex.hpp>

#include <libpq-fe.h>

namespace storages::postgres {

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

OptionsHandle MakeDSNOptions(const Dsn& dsn) {
  char* errmsg = nullptr;
  OptionsHandle opts{PQconninfoParse(dsn.GetUnderlying().c_str(), &errmsg),
                     &PQconninfoFree};

  if (errmsg) {
    InvalidDSN err{DsnMaskPassword(dsn), errmsg};
    PQfreemem(errmsg);
    throw std::move(err);
  }
  return opts;
}

struct HostAndPort {
  using StringList = std::vector<std::string>;

  StringList hosts;
  StringList ports;
};

HostAndPort ParseDSNOptions(
    const Dsn& dsn, std::function<void(PQconninfoOption*)> other_opt_builder =
                        [](PQconninfoOption*) {}) {
  HostAndPort hap;
  auto opts = MakeDSNOptions(dsn);

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

std::string QuoteOptionValue(const char* value) {
  size_t len_with_escapes = 0;
  bool needs_quoting = false;
  for (const char* p = value; *p; ++p) {
    if (*p == ' ' || *p == '\'' || *p == '\\') {
      needs_quoting = true;
      if (*p != ' ') ++len_with_escapes;
    }
    ++len_with_escapes;
  }
  if (!needs_quoting) return std::string(value, len_with_escapes);

  std::string quoted;
  quoted.reserve(len_with_escapes + 2);
  quoted += '\'';
  for (const char* p = value; *p; ++p) {
    switch (*p) {
      case '\\':
      case '\'':
        quoted += '\\';
        [[fallthrough]];
      default:
        quoted += *p;
    }
  }
  quoted += '\'';
  return quoted;
}

}  // namespace

DsnList SplitByHost(const Dsn& dsn) {
  std::ostringstream options;
  const auto hap = ParseDSNOptions(dsn, [&options](PQconninfoOption* opt) {
    // Ignore target_session_attrs
    if (std::strcmp(opt->keyword, "target_session_attrs") != 0) {
      options << " " << opt->keyword << "=" << QuoteOptionValue(opt->val);
    }
  });

  const auto& hosts = hap.hosts;
  const auto& ports = hap.ports;
  DsnList res;
  if (hosts.empty()) {
    // default host, just return the options string
    if (ports.size() > 1)
      throw InvalidDSN{DsnMaskPassword(dsn), "Invalid port options count"};
    if (!ports.empty()) {
      options << " port=" << ports.front();
    }
    res.push_back(Dsn{options.str()});
  } else {
    if (ports.size() > 1 && ports.size() != hosts.size()) {
      throw InvalidDSN{DsnMaskPassword(dsn), "Invalid port options count"};
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
      res.push_back(Dsn{os.str()});
    }
  }

  return res;
}

std::string MakeDsnNick(const Dsn& dsn, bool escape) {
  std::map<std::string, std::string> opt_dict{{"host", kPostgreSQLDefaultHost},
                                              {"port", kPostgreSQLDefaultPort}};

  const auto hap = ParseDSNOptions(dsn, [&opt_dict](PQconninfoOption* opt) {
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
    const auto& key = keys[i];
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

DsnOptions OptionsFromDsn(const Dsn& dsn) {
  DsnOptions options;
  const auto hap = ParseDSNOptions(dsn, [&options](PQconninfoOption* opt) {
    std::string keyword = opt->keyword;
    if (options.dbname.empty() && keyword == "dbname") {
      options.dbname = opt->val;
    }
  });
  options.host = hap.hosts.empty() ? kPostgreSQLDefaultHost : hap.hosts.front();
  if (!hap.ports.empty()) options.port = hap.ports.front();
  return options;
}

std::string GetHostPort(const Dsn& dsn) {
  auto options = OptionsFromDsn(dsn);
  std::string host_port = std::move(options.host);
  if (!options.port.empty()) {
    host_port += ':';
    host_port += options.port;
  }
  return host_port;
}

std::string DsnCutPassword(const Dsn& dsn) {
  OptionsHandle opts = {nullptr, &PQconninfoFree};
  try {
    opts = MakeDSNOptions(dsn);
  } catch (const InvalidDSN&) {
    return {};
  }

  std::string cleared;
  const std::string password = "password";

  auto opt = opts.get();
  while (opt != nullptr && opt->keyword != nullptr) {
    if (opt->val != nullptr && opt->keyword != password) {
      cleared += std::string(opt->keyword) + '=' + std::string(opt->val) + ' ';
    }
    ++opt;
  }
  return cleared;
}

std::string DsnMaskPassword(const Dsn& dsn) {
  static const std::string pg_url_start = "postgresql://";
  static const std::string replace = "${1}***$2";
  if (boost::starts_with(dsn.GetUnderlying(), pg_url_start)) {
    static const boost::regex url_re("^(postgresql://[^:]*:)[^@]+(@)");
    static const boost::regex option_re("\\b(password=)[^&]+");
    auto masked = boost::regex_replace(dsn.GetUnderlying(), url_re, replace);
    masked = boost::regex_replace(masked, option_re, replace);
    return masked;
  } else {
    static const boost::regex option_re(
        R"~(
          (\bpassword\s*=\s*)             # option keyword
          (?:                             # followed by
            (?:'(?:(?:\\['\\])|[^'])+')   # a single-quoted value
                                          # with escaped ' or \ symbols
            |                             # or
            \S+                           # a sequence without spaces
          )
        )~",
        boost::regex_constants::mod_x);
    auto masked = boost::regex_replace(dsn.GetUnderlying(), option_re, replace);
    return masked;
  }
}

std::string EscapeHostName(const std::string& hostname, char escape_char) {
  auto escaped = hostname;
  std::replace_if(escaped.begin(), escaped.end(),
                  [](char c) { return !std::isalpha(c) && !std::isdigit(c); },
                  escape_char);
  return escaped;
}

}  // namespace storages::postgres
