#include "fd_sink.hpp"

#include <spdlog/pattern_formatter.h>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

FdSink::FdSink(fs::blocking::FileDescriptor fd)
    : fd_{std::move(fd)},
      formatter_{std::make_unique<spdlog::pattern_formatter>()} {};

void FdSink::log(const spdlog::details::log_msg& msg) {
  std::lock_guard lock{mutex_};

  spdlog::memory_buf_t formatted;
  formatter_->format(msg, formatted);

  fd_.Write({formatted.data(), formatted.size()});
}

void FdSink::set_pattern(const std::string& pattern) {
  set_formatter(std::make_unique<spdlog::pattern_formatter>(pattern));
}

void FdSink::set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) {
  formatter_ = std::move(sink_formatter);
}

void FdSink::flush() {
  std::lock_guard lock(mutex_);
  if (fd_.IsOpen()) {
    fd_.FSync();
  }
}

FdSink::~FdSink() = default;

std::mutex& FdSink::GetMutex() { return mutex_; }

fs::blocking::FileDescriptor& FdSink::GetFd() { return fd_; }

void FdSink::SetFd(fs::blocking::FileDescriptor&& fd) { std::swap(fd_, fd); }

StdoutSink::StdoutSink()
    : FdSink{fs::blocking::FileDescriptor::AdoptFd(STDOUT_FILENO)} {}

StderrSink::StderrSink()
    : FdSink{fs::blocking::FileDescriptor::AdoptFd(STDERR_FILENO)} {}

void StdoutSink::flush() {}

void StderrSink::flush() {}

StdoutSink::~StdoutSink() {
  // we do not close STDOUT descriptior
  std::move(GetFd()).Release();
}

StderrSink::~StderrSink() {
  // we do not close STDERR descriptior
  std::move(GetFd()).Release();
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
