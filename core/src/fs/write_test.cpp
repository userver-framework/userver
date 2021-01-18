#include <utest/utest.hpp>

#include <boost/filesystem/operations.hpp>

#include <engine/task/task.hpp>
#include <fs/blocking/temp_file.hpp>
#include <fs/blocking/write.hpp>
#include <fs/read.hpp>
#include <fs/write.hpp>

using perms = boost::filesystem::perms;

TEST(AsyncFs, RewriteFileContentsAtomically) {
  const auto file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(file.GetPath(), "old text");

  RunInCoro([&file] {
    const auto new_text = "new text";
    auto& async_tp = engine::current_task::GetTaskProcessor();

    EXPECT_NO_THROW(fs::RewriteFileContentsAtomically(
        async_tp, file.GetPath(), new_text,
        perms::owner_read | perms::owner_write));

    EXPECT_EQ(new_text, fs::ReadFileContents(async_tp, file.GetPath()));
  });
}
