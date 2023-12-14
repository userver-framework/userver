#include <userver/crypto/random.hpp>

#include <cryptopp/osrng.h>

#include <userver/compiler/thread_local.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {

namespace {

#if defined(CRYPTOPP_NO_GLOBAL_BYTE)
using CryptoByte = CryptoPP::byte;
#else
using CryptoByte = ::byte;
#endif

compiler::ThreadLocal local_random_pool = [] {
  return CryptoPP::AutoSeededRandomPool{};
};

}  // namespace

namespace impl {

void GenerateRandomBlock(utils::span<std::byte> buffer) {
  auto random_pool = local_random_pool.Use();
  random_pool->GenerateBlock(reinterpret_cast<CryptoByte*>(buffer.data()),
                             buffer.size());
}

}  // namespace impl

void GenerateRandomBlock(utils::span<char> buffer) {
  return GenerateRandomBlock(utils::as_writable_bytes(buffer));
}

std::string GenerateRandomBlock(std::size_t size) {
  std::string block(size, '\0');
  GenerateRandomBlock(block);
  return block;
}

}  // namespace crypto

USERVER_NAMESPACE_END
