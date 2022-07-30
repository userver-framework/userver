#include <userver/engine/io/pipe.hpp>

#include <fcntl.h>
#include <unistd.h>

#include <array>

#include <userver/engine/io/exception.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>

#include <build_config.hpp>
#include <engine/io/fd_control.hpp>
#include <utils/check_syscall.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {

Pipe::Pipe() {
  std::array<int, 2> pipefd{-1, -1};
  utils::CheckSyscallCustomException<IoSystemError>(
#ifdef HAVE_PIPE2
      ::pipe2(pipefd.data(), O_NONBLOCK | O_CLOEXEC),
#else
      ::pipe(pipefd.data()),
#endif
      "creating pipe");

  utils::FastScopeGuard read_guard(
      [fd = pipefd[0]]() noexcept { ::close(fd); });
  utils::FastScopeGuard write_guard(
      [fd = pipefd[1]]() noexcept { ::close(fd); });

  reader = PipeReader(pipefd[0]);
  read_guard.Release();
  writer = PipeWriter(pipefd[1]);
  write_guard.Release();
}

PipeReader::PipeReader(int fd) : fd_control_(impl::FdControl::Adopt(fd)) {}

bool PipeReader::IsValid() const { return !!fd_control_; }

bool PipeReader::WaitReadable(Deadline deadline) {
  UASSERT(IsValid());
  return fd_control_->Read().Wait(deadline);
}

size_t PipeReader::ReadSome(void* buf, size_t len, Deadline deadline) {
  if (!IsValid()) {
    throw IoException("Attempt to ReadSome from closed pipe end");
  }
  auto& dir = fd_control_->Read();
  impl::Direction::SingleUserGuard guard(dir);
  return dir.PerformIo(guard, &::read, buf, len, impl::TransferMode::kPartial,
                       deadline, "ReadSome from pipe");
}

size_t PipeReader::ReadAll(void* buf, size_t len, Deadline deadline) {
  if (!IsValid()) {
    throw IoException("Attempt to ReadAll from closed pipe end");
  }
  auto& dir = fd_control_->Read();
  impl::Direction::SingleUserGuard guard(dir);
  return dir.PerformIo(guard, &::read, buf, len, impl::TransferMode::kWhole,
                       deadline, "ReadAll from pipe");
}

int PipeReader::Fd() const {
  return fd_control_ ? fd_control_->Fd() : kInvalidFd;
}

int PipeReader::Release() noexcept {
  const int fd = Fd();
  if (IsValid()) {
    fd_control_->Invalidate();
    fd_control_.reset();
  }
  return fd;
}

void PipeReader::Close() { fd_control_.reset(); }

PipeWriter::PipeWriter(int fd) : fd_control_(impl::FdControl::Adopt(fd)) {}

bool PipeWriter::IsValid() const { return !!fd_control_; }

bool PipeWriter::WaitWriteable(Deadline deadline) {
  UASSERT(IsValid());
  return fd_control_->Write().Wait(deadline);
}

size_t PipeWriter::WriteAll(const void* buf, size_t len, Deadline deadline) {
  if (!IsValid()) {
    throw IoException("Attempt to WriteAll to closed pipe end");
  }
  auto& dir = fd_control_->Write();
  impl::Direction::SingleUserGuard guard(dir);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return dir.PerformIo(guard, &::write, const_cast<void*>(buf), len,
                       impl::TransferMode::kWhole, deadline,
                       "WriteAll to pipe");
}

int PipeWriter::Fd() const {
  return fd_control_ ? fd_control_->Fd() : kInvalidFd;
}

int PipeWriter::Release() noexcept {
  const int fd = Fd();
  if (IsValid()) {
    fd_control_->Invalidate();
    fd_control_.reset();
  }
  return fd;
}

void PipeWriter::Close() { fd_control_.reset(); }

}  // namespace engine::io

USERVER_NAMESPACE_END
