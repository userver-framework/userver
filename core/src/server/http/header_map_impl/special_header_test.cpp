#include <userver/utest/utest.hpp>

#include <server/http/header_map_impl/special_header.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http::header_map_impl {

TEST(HeaderMapHasher, SameAtCompileTimeAndRuntime) {
  constexpr auto header = http::kTaxiEnvoyProxyDstVhostHeader;

  ASSERT_GE(header.name.size(), 8);
  ASSERT_NE(header.name.size() % 8, 0);

  constexpr auto compile_time_hash = header.hash;
  const auto runtime_hash =
      RuntimeHasher{}(header.name.data(), header.name.size());

  EXPECT_EQ(compile_time_hash, runtime_hash);
}

}  // namespace server::http::header_map_impl

USERVER_NAMESPACE_END
