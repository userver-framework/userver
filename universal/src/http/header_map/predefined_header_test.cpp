#include <gtest/gtest.h>

#include <fmt/format.h>

#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>

#include <http/header_map/danger.hpp>

USERVER_NAMESPACE_BEGIN

namespace http::headers {

TEST(HeaderMapHasher, SameAtCompileTimeAndRuntime) {
  // Long enough header to go through all paths of hasher implementation.
  constexpr auto header = kXTaxiEnvoyProxyDstVhost;

  constexpr std::string_view header_name{header};
  // load_unaligned(8) at runtime, load_bytes(p, 8) at compile time.
  ASSERT_GE(header_name.size(), 8);
  // load_bytes(p, <8) at both compile and run time.
  ASSERT_NE(header_name.size() % 8, 0);

  const header_map::Danger danger{};

  const auto compile_time_hash = danger.HashKey(header);
  const auto runtime_hash = impl::UnsafeConstexprHasher{}(header_name);

  EXPECT_EQ(compile_time_hash, runtime_hash);
}

TEST(PredefinedHeader, IsFormattable) {
  constexpr auto header = kXRequestApplication;

  const auto header_name_str = fmt::format("{}", header);

  EXPECT_EQ(header_name_str, std::string_view{header});
}

TEST(PredefinedHeader, IsLoggable) {
  LOG_INFO() << "Predefined header '" << kXBackendServer << "' is loggable!";
}

}  // namespace http::headers

USERVER_NAMESPACE_END
