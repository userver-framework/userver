#include <userver/utest/utest.hpp>

#include <boost/filesystem/operations.hpp>

#include <userver/dump/common.hpp>
#include <userver/dump/operations_encrypted.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
const dump::SecretKey kTestKey{"12345678901234567890123456789012"};
}

UTEST(DumpEncFile, Smoke) {
  const auto dir = fs::blocking::TempDirectory::Create();
  const auto path = dir.GetPath() + "/file";

  auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime("dump");
  dump::EncryptedWriter w(path, kTestKey, boost::filesystem::perms::owner_read,
                          scope_time);

  w.Write(1);
  UEXPECT_NO_THROW(w.Finish());

  auto size = boost::filesystem::file_size(path);
  EXPECT_EQ(size, 33);

  dump::EncryptedReader r(path, kTestKey);
  EXPECT_EQ(r.Read<int32_t>(), 1);

  UEXPECT_THROW(r.Read<int32_t>(), dump::Error);

  UEXPECT_NO_THROW(r.Finish());
}

UTEST(DumpEncFile, UnreadData) {
  const auto dir = fs::blocking::TempDirectory::Create();
  const auto path = dir.GetPath() + "/file";

  auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime("dump");
  dump::EncryptedWriter w(path, kTestKey, boost::filesystem::perms::owner_read,
                          scope_time);

  w.Write(1);
  UEXPECT_NO_THROW(w.Finish());

  auto size = boost::filesystem::file_size(path);
  EXPECT_EQ(size, 33);

  dump::EncryptedReader r(path, kTestKey);

  UEXPECT_THROW(r.Finish(), dump::Error);
}

UTEST(DumpEncFile, Long) {
  const auto dir = fs::blocking::TempDirectory::Create();
  const auto path = dir.GetPath() + "/file";

  auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime("dump");
  dump::EncryptedWriter w(path, kTestKey, boost::filesystem::perms::owner_read,
                          scope_time);

  for (int i = 0; i < 256; i++) w.Write(i);
  UEXPECT_NO_THROW(w.Finish());

  auto size = boost::filesystem::file_size(path);
  EXPECT_EQ(size, 416);

  dump::EncryptedReader r(path, kTestKey);
  for (int i = 0; i < 256; i++) EXPECT_EQ(r.Read<int32_t>(), i);

  UEXPECT_THROW(r.Read<int32_t>(), dump::Error);

  UEXPECT_NO_THROW(r.Finish());
}

USERVER_NAMESPACE_END
