#include <userver/utest/utest.hpp>

#include <server/http/header_map_impl/special_header.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http::header_map_impl {

TEST(HeaderMapHasher, SameAtCompileTimeAndRuntime) {
  // Long enough header to go through all paths of hasher implementation.
  constexpr auto header = http::kXTaxiEnvoyProxyDstVhostHeader;

  // load_unaligned(8) at runtime, load_bytes(p, 8) at compile time.
  ASSERT_GE(header.name.size(), 8);
  // load_bytes(p, <8) at both compile and run time.
  ASSERT_NE(header.name.size() % 8, 0);

  constexpr auto compile_time_hash = header.hash;
  const auto runtime_hash =
      RuntimeHasher{}(header.name.data(), header.name.size());

  EXPECT_EQ(compile_time_hash, runtime_hash);
}

}  // namespace server::http::header_map_impl

USERVER_NAMESPACE_END
