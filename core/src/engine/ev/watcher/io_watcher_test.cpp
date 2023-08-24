#include <engine/ev/watcher/io_watcher.hpp>

#include <fcntl.h>
#include <sys/param.h>

#include <engine/ev/thread.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

#if defined(BSD) && !defined(__APPLE__)
UTEST(IoWatcher, DISABLED_DevNull) {
#else
UTEST(IoWatcher, DevNull) {
#endif
  LOG_DEBUG() << "Opening /dev/null";
  engine::ev::Thread thread("test_thread",
                            engine::ev::Thread::RegisterEventMode::kImmediate);
  engine::ev::ThreadControl thread_control(thread);

  int fd = open("/dev/null", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
  EXPECT_NE(-1, fd);

  std::mutex mutex;
  std::condition_variable cv;
  std::atomic_int counter{0};
  std::atomic_bool done{false};

  engine::ev::IoWatcher watcher(thread_control);
  watcher.SetFd(fd);
  watcher.ReadAsync([&counter, &mutex, &cv, &fd, &done](std::error_code) {
    std::lock_guard<std::mutex> lock(mutex);

    char c{};
    int rc = read(fd, &c, 1);
    if (rc == 0) done = true;
    if (rc > 0) counter += rc;
    EXPECT_EQ(rc, 0);

    cv.notify_all();
  });

  {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait_for(lock, std::chrono::milliseconds(200),
                [&done]() { return done.load(); });
  }

  EXPECT_EQ(done, true);
  EXPECT_EQ(counter, 0);
}

USERVER_NAMESPACE_END
