#include <utest/utest.hpp>

#include <boost/filesystem/operations.hpp>

#include <cache/dump/common.hpp>
#include <cache/dump/operations_encrypted.hpp>
#include <fs/blocking/temp_directory.hpp>

using namespace cache::dump;

namespace {
const SecretKey kTestKey{"12345678901234567890123456789012"};
}

TEST(CacheDumpEncFile, Smoke) {
  const auto dir = fs::blocking::TempDirectory::Create();
  const auto path = dir.GetPath() + "/file";

  EncryptedWriter w(path, kTestKey, boost::filesystem::perms::owner_read);

  w.Write(1);
  EXPECT_NO_THROW(w.Finish());

  auto size = boost::filesystem::file_size(path);
  EXPECT_EQ(size, 33);

  EncryptedReader r(path, kTestKey);
  EXPECT_EQ(r.Read<int32_t>(), 1);

  EXPECT_THROW(r.Read<int32_t>(), Error);

  EXPECT_NO_THROW(r.Finish());
}

TEST(CacheDumpEncFile, UnreadData) {
  const auto dir = fs::blocking::TempDirectory::Create();
  const auto path = dir.GetPath() + "/file";

  EncryptedWriter w(path, kTestKey, boost::filesystem::perms::owner_read);

  w.Write(1);
  EXPECT_NO_THROW(w.Finish());

  auto size = boost::filesystem::file_size(path);
  EXPECT_EQ(size, 33);

  EncryptedReader r(path, kTestKey);

  EXPECT_THROW(r.Finish(), Error);
}

TEST(CacheDumpEncFile, Long) {
  const auto dir = fs::blocking::TempDirectory::Create();
  const auto path = dir.GetPath() + "/file";

  EncryptedWriter w(path, kTestKey, boost::filesystem::perms::owner_read);

  for (int i = 0; i < 256; i++) w.Write(i);
  EXPECT_NO_THROW(w.Finish());

  auto size = boost::filesystem::file_size(path);
  EXPECT_EQ(size, 416);

  EncryptedReader r(path, kTestKey);
  for (int i = 0; i < 256; i++) EXPECT_EQ(r.Read<int32_t>(), i);

  EXPECT_THROW(r.Read<int32_t>(), Error);

  EXPECT_NO_THROW(r.Finish());
}
