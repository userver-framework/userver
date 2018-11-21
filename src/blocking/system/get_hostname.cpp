#include <blocking/system/get_hostname.hpp>

#include <limits.h>
#include <unistd.h>
#include <system_error>

namespace blocking::system {

std::string GetRealHostName() {
  char host_name[HOST_NAME_MAX];
  if (::gethostname(host_name, HOST_NAME_MAX) == -1) {
    const auto code = std::make_error_code(std::errc(errno));
    throw std::system_error(code, "Error while getting hostname");
  }

  return host_name;
}

}  // namespace blocking::system
