#include <blocking/system/get_hostname.hpp>

#include <limits.h>
#include <unistd.h>
#include <system_error>

namespace blocking::system {

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
  char host_name[kHostNameMax];
  if (::gethostname(host_name, kHostNameMax) == -1) {
    const auto code = std::make_error_code(std::errc(errno));
    throw std::system_error(code, "Error while getting hostname");
  }

  return host_name;
}

}  // namespace blocking::system
