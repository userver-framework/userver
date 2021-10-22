#include <userver/utest/utest.hpp>

#include <boost/filesystem/operations.hpp>

#include <userver/engine/task/task.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/fs/read.hpp>
#include <userver/fs/write.hpp>

USERVER_NAMESPACE_BEGIN

using perms = boost::filesystem::perms;

UTEST(AsyncFs, RewriteFileContentsAtomically) {
  const auto file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(file.GetPath(), "old text");

  const auto new_text = "new text";
  auto& async_tp = engine::current_task::GetTaskProcessor();

  EXPECT_NO_THROW(fs::RewriteFileContentsAtomically(
      async_tp, file.GetPath(), new_text,
      perms::owner_read | perms::owner_write));

  EXPECT_EQ(new_text, fs::ReadFileContents(async_tp, file.GetPath()));
}

USERVER_NAMESPACE_END
