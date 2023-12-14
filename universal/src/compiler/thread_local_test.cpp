#include <userver/compiler/thread_local.hpp>

#include <random>
#include <string>
#include <string_view>
#include <unordered_set>

#include <gtest/gtest.h>

#include <userver/fs/blocking/file_descriptor.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/utils/rand.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

#ifdef USERVER_IMPL_UNEVALUATED_LAMBDAS
/// [sample definition]
// NOTE: the thread-local itself should be:
// * NEVER `thread_local`
// * `static` where appropriate
// * `inline` where appropriate
compiler::ThreadLocal<std::string> local_buffer;
/// [sample definition]
#else
compiler::ThreadLocal local_buffer = [] { return std::string{}; };
#endif

/// [sample]
void WriteAll(fs::blocking::FileDescriptor& fd,
              utils::span<const std::string_view> data) {
  // If we just write one item in `data` after the other, we might perform too
  // many syscall-s, because each of the strings may be short.
  //
  // So let's somehow concatenate `data` into a single string. But now there
  // may be too many allocations if such an operation is frequent.
  //
  // Solution: use a thread_local buffer to only allocate once per thread.

  // NOTE: the scope should be non-static, non-thread_local.
  auto buffer = local_buffer.Use();

  buffer->clear();
  for (const auto piece : data) {
    buffer->append(piece);
  }
  fd.Write(*buffer);
}
/// [sample]

TEST(ThreadLocal, Sample) {
  const auto temp_file = fs::blocking::TempFile::Create();
  {
    auto fd = fs::blocking::FileDescriptor::Open(
        temp_file.GetPath(), fs::blocking::OpenFlag::kWrite);
    const std::string_view data[]{"foo", "bar", "baz"};
    WriteAll(fd, data);
  }
  EXPECT_EQ(fs::blocking::ReadFileContents(temp_file.GetPath()), "foobarbaz");
}

/// [sample factory]
// NOTE: if you need thread-local randomness, don't roll your own!
// See <userver/utils/rand.hpp> and <userver/crypto/random.hpp> first!
compiler::ThreadLocal local_rng = [] {
  return std::minstd_rand{utils::Rand()};
};

std::uint32_t MyRand() {
  auto rng = local_rng.Use();
  return std::uniform_int_distribution<std::uint32_t>{}(*rng);
}
/// [sample factory]

TEST(ThreadLocal, SampleFactory) {
  static constexpr int kIterations = 100;
  std::unordered_set<std::uint32_t> values;
  for (int i = 0; i < kIterations; ++i) {
    values.insert(MyRand());
  }
  // The probability of this test failing is on the order of 2^(-3200).
  EXPECT_GE(values.size(), 2);
}

}  // namespace

USERVER_NAMESPACE_END
