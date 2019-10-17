#include "openssl_lock.hpp"

#include <iostream>
#include <mutex>
#include <vector>

#include <openssl/crypto.h>

namespace utils {

#if OPENSSL_VERSION_NUMBER >= 0x010100000L

// OpenSSL >= 1.1 has builtin threading support
OpensslLock::OpensslLock() = default;
void OpensslLock::Init() {}
void OpensslLock::LockingFunction(int, int, const char*, int) {}

#else

void OpensslLock::LockingFunction(int mode, int n, const char*, int) {
  static std::vector<std::mutex> mutexes(CRYPTO_num_locks());

  if (n < 0 || n >= static_cast<int>(mutexes.size())) {
    std::cerr << "OpensslLock::LockingFunction called with invalid n: " << n
              << ", max=" << mutexes.size() << std::endl;
    abort();
  }

  if (mode & CRYPTO_LOCK)
    mutexes[n].lock();
  else
    mutexes[n].unlock();
}

OpensslLock::OpensslLock() {
  CRYPTO_set_locking_callback(&OpensslLock::LockingFunction);
}

void OpensslLock::Init() { [[maybe_unused]] static OpensslLock lock; }

#endif

}  // namespace utils
