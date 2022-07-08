#include <userver/hostinfo/blocking/get_hostname.hpp>

#include <unistd.h>

#include <array>
#include <climits>
#include <system_error>

USERVER_NAMESPACE_BEGIN

namespace hostinfo::blocking {

namespace {

#ifdef HOST_NAME_MAX
const auto kHostNameMax = HOST_NAME_MAX;
#elif defined(_POSIX_HOST_NAME_MAX)
const auto kHostNameMax = _POSIX_HOST_NAME_MAX;
#elif defined(MAXHOSTNAMELEN)
const auto kHostNameMax = MAXHOSTNAMELEN;
#endif /* HOST_NAME_MAX */

}  // namespace

std::string GetRealHostName() {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init): performance
  std::array<char, kHostNameMax> host_name;
  if (::gethostname(host_name.data(), host_name.size()) == -1) {
    const auto code = std::make_error_code(std::errc{errno});
    throw std::system_error(code, "Error while getting hostname");
  }

  return host_name.data();
}

}  // namespace hostinfo::blocking

USERVER_NAMESPACE_END
