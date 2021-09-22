#include <clients/dns/file_resolver.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <algorithm>
#include <cctype>
#include <fstream>

#include <userver/logging/log.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/text.hpp>

namespace clients::dns {
namespace {

struct Token {
  char* data{nullptr};
  size_t length{0};

  bool IsEmpty() { return !data || !length; }
};

Token GetToken(char* str) {
  // skip whitespace
  while (*str && utils::text::IsAsciiSpace(*str)) ++str;
  if (!*str) return {};

  Token token{str};

  // find token end
  while (*str && !utils::text::IsAsciiSpace(*str)) ++str;
  token.length = str - token.data;
  return token;
}

engine::io::Sockaddr TryParseAddr(char* addr_str) {
  engine::io::Sockaddr addr;

  // Attempt to parse as an IPv6 address first, then as an IPv4 address
  if (::inet_pton(AF_INET6, addr_str,
                  &addr.As<struct sockaddr_in6>()->sin6_addr) == 1) {
    addr.Data()->sa_family = AF_INET6;
  } else if (::inet_pton(AF_INET, addr_str,
                         &addr.As<struct sockaddr_in>()->sin_addr) == 1) {
    addr.Data()->sa_family = AF_INET;
  }
  return addr;
}

AddrVector FilterByDomain(const AddrVector& src,
                          engine::io::AddrDomain domain) {
  if (domain == engine::io::AddrDomain::kUnspecified) return src;

  AddrVector filtered;
  for (const auto& addr : src) {
    if (addr.Domain() == domain) {
      filtered.push_back(addr);
    }
  }
  return filtered;
}

void SortAddrs(AddrVector& addrs) {
  std::stable_partition(
      addrs.begin(), addrs.end(), [](const engine::io::Sockaddr& addr) {
        return addr.Domain() == engine::io::AddrDomain::kInet6;
      });
}

}  // namespace

FileResolver::FileResolver(Config config)
    : config_(std::move(config)),
      update_task_("file-resolver-updater",
                   utils::PeriodicTask::Settings{config_.update_interval, {}},
                   [this] { ReloadHosts(); }) {
  ReloadHosts();
}

AddrVector FileResolver::Resolve(const std::string& name,
                                 engine::io::AddrDomain domain) const {
  auto hosts = hosts_.Read();
  auto it = hosts->find(name);
  if (it == hosts->end()) return {};
  return FilterByDomain(it->second, domain);
}

void FileResolver::ReloadHosts() {
  utils::Async(config_.fs_task_processor, "file-resolver-reload", [this] {
    LOG_DEBUG() << "Reloading static hosts mapping from " << config_.path;
    std::ifstream hosts_fs(config_.path);
    if (!hosts_fs) {
      LOG_WARNING() << "Cannot open " << config_.path << ", skipping update";
      return;
    }
    std::string line;
    std::unordered_map<std::string, AddrVector> new_hosts;
    while (std::getline(hosts_fs, line)) {
      // trim comment
      const auto comment_pos = line.find('#');
      if (comment_pos != std::string::npos) {
        line.resize(comment_pos);
      }

      // parse address
      auto token = GetToken(line.data());
      if (token.IsEmpty()) continue;
      // use the fact that it is followed by whitespace (or by nothing)
      // to zero-terminate in-place
      token.data[token.length] = '\0';
      ++token.length;
      auto addr = TryParseAddr(token.data);
      LOG_TRACE() << "Parsed address " << addr;
      if (addr.Domain() == engine::io::AddrDomain::kUnspecified) continue;

      // parse aliases and bind them
      token = GetToken(token.data + token.length);
      while (!token.IsEmpty()) {
        std::string name{token.data, token.length};
        LOG_TRACE() << "Parsed name '" << name << '\'';
        new_hosts[std::move(name)].push_back(addr);
        token = GetToken(token.data + token.length);
      }
    }

    for (auto& [_, addrs] : new_hosts) SortAddrs(addrs);
    LOG_DEBUG() << "Loaded static hosts mapping with " << new_hosts.size()
                << " names";
    hosts_.Assign(std::move(new_hosts));
  }).Get();
}

}  // namespace clients::dns
