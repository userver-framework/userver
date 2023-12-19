#include <userver/hostinfo/blocking/get_hostname.hpp>

#include <unistd.h>

#include <array>
#include <climits>
#include <system_error>

#include <userver/compiler/impl/constexpr.hpp>

USERVER_NAMESPACE_BEGIN

namespace hostinfo::blocking {

namespace {

#ifdef HOST_NAME_MAX
constexpr std::size_t kHostNameMax = HOST_NAME_MAX;
#elif defined(_POSIX_HOST_NAME_MAX)
constexpr std::size_t kHostNameMax = _POSIX_HOST_NAME_MAX;
#elif defined(MAXHOSTNAMELEN)
constexpr std::size_t kHostNameMax = MAXHOSTNAMELEN;
#endif /* HOST_NAME_MAX */

USERVER_IMPL_CONSTINIT std::array<char, kHostNameMax> host_name{};

std::string_view DoGetRealHostName() {
  if (::gethostname(host_name.data(), host_name.size()) == -1) {
    const auto code = std::make_error_code(std::errc{errno});
    throw std::system_error(code, "Error while getting hostname");
  }
  return std::string_view{host_name.data()};
}

std::string_view GetRealHostNameView() {
  static const std::string_view host_name_view = DoGetRealHostName();
  return host_name_view;
}

// Cache real host name at static initialization time to avoid a syscall
// and mutex locks later.
const bool init_host_name = (GetRealHostNameView(), false);

}  // namespace

std::string GetRealHostName() {
  static_cast<void>(init_host_name);
  return std::string{GetRealHostNameView()};
}

}  // namespace hostinfo::blocking

USERVER_NAMESPACE_END
