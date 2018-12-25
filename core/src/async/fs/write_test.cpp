#include <utest/utest.hpp>

#include <boost/filesystem/operations.hpp>

#include <async/fs/read.hpp>
#include <async/fs/write.hpp>
#include <blocking/fs/file_descriptor.hpp>
#include <engine/task/task.hpp>

using perms = boost::filesystem::perms;

TEST(AsyncFs, RewriteFileContentsAtomically) {
  auto fd = blocking::fs::FileDescriptor::CreateTempFile(
      boost::filesystem::temp_directory_path().string());

  auto path = fd.GetPath();
  static const auto str = "old text";

  EXPECT_NO_THROW(fd.Write(str));
  fd.Close();

  RunInCoro([&path] {
    const auto str2 = "new text";
    auto& async_tp = engine::current_task::GetTaskProcessor();
    EXPECT_NO_THROW(async::fs::RewriteFileContentsAtomically(
        async_tp, path, str2, perms::owner_read | perms::owner_write));

    EXPECT_EQ(str2, async::fs::ReadFileContents(async_tp, path));
  });
}
