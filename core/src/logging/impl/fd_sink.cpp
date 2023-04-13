#include "fd_sink.hpp"

#include <spdlog/pattern_formatter.h>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

FdSink::FdSink(fs::blocking::FileDescriptor fd) : fd_{std::move(fd)} {};

void FdSink::Write(std::string_view log) { fd_.Write(log); }

void FdSink::flush() {
  std::lock_guard lock(GetMutex());
  if (fd_.IsOpen()) {
    fd_.FSync();
  }
}

FdSink::~FdSink() = default;

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
