#include "fd_sink.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

FdSink::FdSink(fs::blocking::FileDescriptor fd)
    : fd_{std::move(fd)}
{}

void FdSink::Write(std::span<const struct iovec> logs) { fd_.Write(logs); }

FdSink::~FdSink() = default;

fs::blocking::FileDescriptor& FdSink::GetFd() { return fd_; }

void FdSink::SetFd(fs::blocking::FileDescriptor&& fd) { std::swap(fd_, fd); }

UnownedFdSink::UnownedFdSink(int fd)
    : FdSink(fs::blocking::FileDescriptor::AdoptFd(fd))
{}

UnownedFdSink::~UnownedFdSink() { std::move(GetFd()).Release(); }

}  // namespace logging::impl

USERVER_NAMESPACE_END
