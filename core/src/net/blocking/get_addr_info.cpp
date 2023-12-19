#include <netdb.h>
#include <sys/socket.h>

#include <userver/net/blocking/get_addr_info.hpp>
#include <userver/utils/assert.hpp>
#include <utils/check_syscall.hpp>

USERVER_NAMESPACE_BEGIN

namespace net::blocking {

std::vector<engine::io::Sockaddr> GetAddrInfo(std::string_view host,
                                              const char* service_or_port) {
  UINVARIANT(!host.empty(), "The host should not be empty");
  UINVARIANT(service_or_port != nullptr,
             "The service_or_port should not be null");
  std::vector<engine::io::Sockaddr> result{};

  struct addrinfo hints {};
  std::memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;

  struct addrinfo* temp_result{};

  auto rv = ::getaddrinfo(host.data(), service_or_port, &hints, &temp_result);

  std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> addrinfo_result(
      temp_result, &::freeaddrinfo);

  if (rv != 0) {
    throw std::runtime_error(
        fmt::format("::getaddrinfo failed: {}", gai_strerror(rv)));
  }
  for (auto* rp = addrinfo_result.get(); rp != nullptr; rp = rp->ai_next) {
    auto addr = engine::io::Sockaddr(rp->ai_addr);

    result.push_back(addr);
  }
  return result;
}

}  // namespace net::blocking

USERVER_NAMESPACE_END
