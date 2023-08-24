#include <userver/fs/blocking/file_descriptor.hpp>

#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <system_error>
#include <utility>

#include <boost/filesystem/operations.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <utils/check_syscall.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs::blocking {

namespace {

int ToNative(OpenMode flags) {
  int result = 0;

  // using file descriptor without this flag is a security risk
  // read: man 2 open
  result |= O_CLOEXEC;

  if ((flags & OpenFlag::kRead) && (flags & OpenFlag::kWrite)) {
    result |= O_RDWR;
  } else if (flags & OpenFlag::kRead) {
    result |= O_RDONLY;
  } else if (flags & OpenFlag::kWrite) {
    result |= O_WRONLY;
  } else {
    UINVARIANT(false, "Specify at least one of kRead, kWrite in OpenFlags");
  }

  if (flags & OpenFlag::kCreateIfNotExists) {
    UINVARIANT(flags & OpenFlag::kWrite,
               "Cannot use kCreateIfNotExists without kWrite in OpenFlags");
    result |= O_CREAT;
  }

  if (flags & OpenFlag::kExclusiveCreate) {
    UINVARIANT(flags & OpenFlag::kWrite,
               "Cannot use kCreateIfNotExists without kWrite in OpenFlags");
    result |= O_CREAT | O_EXCL;
  }

  if (flags & OpenFlag::kTruncate) {
    UINVARIANT(flags & OpenFlag::kWrite,
               "Cannot use kTruncate without kWrite in OpenFlags");
    UINVARIANT(!(flags & OpenFlag::kExclusiveCreate),
               "Cannot use kTruncate with kExclusiveCreate in OpenFlags");
    result |= O_TRUNC;
  }

  if (flags & OpenFlag::kAppend) {
    UINVARIANT(flags & OpenFlag::kWrite,
               "Cannot use kAppend without kWrite in OpenFlags");
    result |= O_APPEND;
  }

  return result;
}

auto GetFileStats(int fd) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  struct ::stat result;
  utils::CheckSyscall(::fstat(fd, &result), "calling ::fstat");
  return result;
}

constexpr int kNoFd = -1;

}  // namespace

FileDescriptor::FileDescriptor(int fd) : fd_(fd) { UASSERT(fd != kNoFd); }

FileDescriptor FileDescriptor::Open(const std::string& path, OpenMode flags,
                                    boost::filesystem::perms perms) {
  UASSERT(!path.empty());
  const auto fd = utils::CheckSyscall(
      ::open(path.c_str(), ToNative(flags), perms), "opening file '{}'", path);
  return FileDescriptor{fd};
}

FileDescriptor FileDescriptor::AdoptFd(int fd) noexcept {
  UASSERT_MSG(::fcntl(fd, F_GETFD) != -1, "This file descriptor is not valid");
  return FileDescriptor{fd};
}

FileDescriptor FileDescriptor::OpenDirectory(const std::string& path) {
  UASSERT(!path.empty());
  const auto fd =
      utils::CheckSyscall(::open(path.c_str(), O_RDONLY | O_DIRECTORY),
                          "opening directory '{}'", path);
  return FileDescriptor{fd};
}

FileDescriptor::FileDescriptor(FileDescriptor&& other) noexcept
    : fd_(std::exchange(other.fd_, kNoFd)) {}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept {
  if (&other != this) {
    FileDescriptor temp = std::move(*this);
    fd_ = std::exchange(other.fd_, kNoFd);
  }
  return *this;
}

FileDescriptor::~FileDescriptor() {
  if (IsOpen()) {
    try {
      std::move(*this).Close();
    } catch (const std::exception& e) {
      LOG_ERROR() << e;
    }
  }
}

bool FileDescriptor::IsOpen() const { return fd_ != kNoFd; }

void FileDescriptor::Close() && {
  const auto fd = std::exchange(fd_, kNoFd);
  utils::CheckSyscall(::close(fd), "calling ::close");
}

int FileDescriptor::GetNative() const { return fd_; }

int FileDescriptor::Release() && { return std::exchange(fd_, kNoFd); }

// NOLINTNEXTLINE(readability-make-member-function-const)
void FileDescriptor::Write(std::string_view contents) {
  const char* buffer = contents.data();
  size_t len = contents.size();

  while (len > 0) {
    ssize_t s = ::write(fd_, buffer, len);
    if (s < 0) {
      if (errno == EAGAIN || errno == EINTR) continue;

      const auto code = std::make_error_code(std::errc{errno});
      throw std::system_error(code, "calling ::write");
    }

    buffer += s;
    len -= s;
  }
}

// NOLINTNEXTLINE(readability-make-member-function-const)
std::size_t FileDescriptor::Read(char* buffer, std::size_t max_size) {
  while (true) {
    const ::ssize_t s = ::read(fd_, buffer, max_size);
    if (s < 0) {
      if (errno == EAGAIN || errno == EINTR) continue;

      const auto code = std::make_error_code(std::errc{errno});
      throw std::system_error(code, "calling ::read");
    }
    return s;
  }
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void FileDescriptor::Seek(std::size_t offset_in_bytes) {
  while (true) {
    const auto s = ::lseek(fd_, offset_in_bytes, SEEK_SET);
    if (s < 0) {
      if (errno == EAGAIN || errno == EINTR) continue;

      const auto code = std::make_error_code(std::errc{errno});
      throw std::system_error(code, "calling ::lseek");
    }

    UASSERT(static_cast<std::size_t>(s) == offset_in_bytes);
    break;
  }
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void FileDescriptor::FSync() {
  utils::CheckSyscall(::fsync(fd_), "calling ::fsync");
}

std::size_t FileDescriptor::GetSize() const {
  return GetFileStats(fd_).st_size;
}

}  // namespace fs::blocking

USERVER_NAMESPACE_END
