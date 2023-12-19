#include <crypto/openssl.hpp>

#include <cerrno>
#include <cstdio>
#include <mutex>
#include <vector>

#if OPENSSL_VERSION_NUMBER < 0x010100000L
#include <openssl/conf.h>
#endif

#include <openssl/crypto.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

USERVER_NAMESPACE_BEGIN

namespace crypto::impl {
namespace {

// OpenSSL >= 1.1 has builtin threading support
#if OPENSSL_VERSION_NUMBER < 0x010100000L
void ThreadIdCallback(CRYPTO_THREADID* thread_id) noexcept {
  // default OpenSSL implementation
  CRYPTO_THREADID_set_pointer(thread_id, static_cast<void*>(&errno));
}

unsigned long LegacyIdCallback() noexcept {
  // should never be used
  std::fprintf(stderr, "crypto::impl::LegacyIdCallback used\n");
  abort();
}

void LockingCallback(int mode, int n, const char*, int) noexcept {
  static std::vector<std::mutex> mutexes(CRYPTO_num_locks());

  if (n < 0 || n >= static_cast<int>(mutexes.size())) {
    std::fprintf(
        stderr,
        "crypto::impl::LockingCallback called with invalid n: %d, max=%d\n", n,
        static_cast<int>(mutexes.size()));
    abort();
  }

  if (mode & CRYPTO_LOCK)
    mutexes[n].lock();
  else
    mutexes[n].unlock();
}
#endif

void TrySetDefaultEngineRdrand() noexcept {
  ENGINE_load_rdrand();
  ENGINE* eng = ENGINE_by_id("rdrand");
  if (!eng) {
    return;
  }
  if (ENGINE_init(eng) == 1) {
    ENGINE_set_default(eng, ENGINE_METHOD_RAND);
    ENGINE_finish(eng);
  }
  ENGINE_free(eng);
  ERR_clear_error();
}

}  // namespace

Openssl::Openssl() noexcept {
#if OPENSSL_VERSION_NUMBER >= 0x010100000L
  OPENSSL_init_ssl(OPENSSL_INIT_LOAD_CONFIG, nullptr);
#else
  SSL_library_init();
  SSL_load_error_strings();
  OPENSSL_config(nullptr);
  CRYPTO_THREADID_set_callback(&ThreadIdCallback);
  CRYPTO_set_locking_callback(&LockingCallback);
  // never used when THREADID is set, but is often checked by libs
  CRYPTO_set_id_callback(&LegacyIdCallback);
#endif

  TrySetDefaultEngineRdrand();
}

void Openssl::Init() noexcept { [[maybe_unused]] static Openssl lock; }

}  // namespace crypto::impl

USERVER_NAMESPACE_END
