#include <utils/sys_info.hpp>

#include <unistd.h>

USERVER_NAMESPACE_BEGIN

namespace utils::sys_info {

std::size_t GetPageSize() {
    static const std::size_t kPageSize = sysconf(_SC_PAGESIZE);

    return kPageSize;
}

}  // namespace utils::sys_info

USERVER_NAMESPACE_END
