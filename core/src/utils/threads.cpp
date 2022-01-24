#include <utils/threads.hpp>

#ifdef __APPLE__
#include <pthread.h>
#else
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#endif

USERVER_NAMESPACE_BEGIN

namespace utils {

#ifdef __linux__
// Returns thread id
pid_t GetTid() { return static_cast<pid_t>(syscall(SYS_gettid)); }
#endif

bool IsMainThread() {
#ifdef __APPLE__
  return !!pthread_main_np();
#else
  return getpid() == GetTid();
#endif
}

}  // namespace utils

USERVER_NAMESPACE_END
