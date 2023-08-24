#include <clients/dns/file_resolver.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cctype>
#include <fstream>

#include <clients/dns/helpers.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {
namespace {

struct Token {
  char* data{nullptr};
  size_t length{0};

  bool IsEmpty() const { return !data || !length; }
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

}  // namespace

FileResolver::FileResolver(engine::TaskProcessor& fs_task_processor,
                           std::string path,
                           std::chrono::milliseconds update_interval)
    : fs_task_processor_(fs_task_processor),
      path_(std::move(path)),
      update_task_("file-resolver-updater",
                   utils::PeriodicTask::Settings{update_interval, {}},
                   [this] { ReloadHosts(); }) {
  ReloadHosts();
}

AddrVector FileResolver::Resolve(const std::string& name) const {
  auto hosts = hosts_.Read();
  return utils::FindOrDefault(*hosts, name);
}

void FileResolver::ReloadHosts() {
  utils::Async(fs_task_processor_, "file-resolver-reload", [this] {
    LOG_DEBUG() << "Reloading static hosts mapping from " << path_;
    std::ifstream hosts_fs(path_);
    if (!hosts_fs) {
      LOG_WARNING() << "Cannot open " << path_ << ", skipping update";
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

    for (auto& [_, addrs] : new_hosts) impl::SortAddrs(addrs);
    LOG_DEBUG() << "Loaded static hosts mapping with " << new_hosts.size()
                << " names";
    hosts_.Assign(std::move(new_hosts));
  }).Get();
}

}  // namespace clients::dns

USERVER_NAMESPACE_END
