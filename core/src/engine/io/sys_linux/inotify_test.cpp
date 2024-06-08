#ifdef __linux__
#include <userver/utest/utest.hpp>

#include <userver/engine/io/sys_linux/inotify.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/fs/blocking/file_descriptor.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/logging/log.hpp>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

USERVER_NAMESPACE_BEGIN

UTEST(Inotify, File) {
  auto tmp = fs::blocking::TempFile::Create();
  const auto& path = tmp.GetPath();

  namespace sys_linux = engine::io::sys_linux;
  sys_linux::Inotify inotify;
  inotify.AddWatch(
      path, {sys_linux::EventType::kOpen, sys_linux::EventType::kAccess,
             sys_linux::EventType::kCloseWrite, sys_linux::EventType::kModify});

  {
    auto fd = fs::blocking::FileDescriptor::Open(
        path, {fs::blocking::OpenFlag::kRead, fs::blocking::OpenFlag::kWrite});
    fd.Write("1");
    fd.Seek(0);
    char buffer[1];
    fd.Read(buffer, sizeof(buffer));
  }

  auto event = inotify.Poll({});
  ASSERT_TRUE(event);
  EXPECT_EQ(event, (sys_linux::Event{path, sys_linux::EventType::kOpen}));

  event = inotify.Poll({});
  ASSERT_TRUE(event);
  EXPECT_EQ(event, (sys_linux::Event{path, sys_linux::EventType::kModify}));

  event = inotify.Poll({});
  ASSERT_TRUE(event);
  EXPECT_EQ(event, (sys_linux::Event{path, sys_linux::EventType::kAccess}));

  event = inotify.Poll({});
  ASSERT_TRUE(event);
  EXPECT_EQ(event, (sys_linux::Event{path, sys_linux::EventType::kCloseWrite}));
}

UTEST(Inotify, Directory) {
  auto tmp = fs::blocking::TempDirectory::Create();
  const auto& path = tmp.GetPath();

  fs::blocking::RewriteFileContents(path + "/1", "contents");

  namespace sys_linux = engine::io::sys_linux;
  sys_linux::Inotify inotify;
  inotify.AddWatch(path, {
                             sys_linux::EventType::kOpen,
                             sys_linux::EventType::kAccess,
                             sys_linux::EventType::kCloseWrite,
                             sys_linux::EventType::kCloseNoWrite,
                             sys_linux::EventType::kModify,
                             sys_linux::EventType::kMovedFrom,
                             sys_linux::EventType::kMovedTo,
                         });

  {
    fs::blocking::RewriteFileContents(path + "/2", "contents");
    auto contents = fs::blocking::ReadFileContents(path + "/2");
  }

  auto event = inotify.Poll({});
  ASSERT_TRUE(event);
  EXPECT_EQ(event,
            (sys_linux::Event{path + "/2", sys_linux::EventType::kOpen}));

  event = inotify.Poll({});
  ASSERT_TRUE(event);
  EXPECT_EQ(event,
            (sys_linux::Event{path + "/2", sys_linux::EventType::kModify}));

  event = inotify.Poll({});
  ASSERT_TRUE(event);
  EXPECT_EQ(event,
            (sys_linux::Event{path + "/2", sys_linux::EventType::kCloseWrite}));

  event = inotify.Poll({});
  ASSERT_TRUE(event);
  EXPECT_EQ(event,
            (sys_linux::Event{path + "/2", sys_linux::EventType::kOpen}));

  event = inotify.Poll({});
  ASSERT_TRUE(event);
  EXPECT_EQ(event,
            (sys_linux::Event{path + "/2", sys_linux::EventType::kAccess}));

  event = inotify.Poll({});
  ASSERT_TRUE(event);
  EXPECT_EQ(event, (sys_linux::Event{path + "/2",
                                     sys_linux::EventType::kCloseNoWrite}));

  fs::blocking::Rename(path + "/2", path + "/3");

  event = inotify.Poll({});
  ASSERT_TRUE(event);
  EXPECT_EQ(event,
            (sys_linux::Event{path + "/2", sys_linux::EventType::kMovedFrom}));

  event = inotify.Poll({});
  ASSERT_TRUE(event);
  EXPECT_EQ(event,
            (sys_linux::Event{path + "/3", sys_linux::EventType::kMovedTo}));
}

UTEST(Inotify, TaskCancel) {
  namespace sys_linux = engine::io::sys_linux;
  sys_linux::Inotify inotify;
  inotify.AddWatch(
      "/", {sys_linux::EventType::kOpen, sys_linux::EventType::kAccess,
            sys_linux::EventType::kCloseWrite, sys_linux::EventType::kModify});

  engine::current_task::GetCancellationToken().RequestCancel();

  auto event = inotify.Poll({});
  ASSERT_FALSE(event);
}

USERVER_NAMESPACE_END
#endif
