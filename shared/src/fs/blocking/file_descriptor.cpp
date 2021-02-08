#include <fs/blocking/file_descriptor.hpp>

#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <system_error>
#include <utility>

#include <boost/filesystem/operations.hpp>

#include <logging/log.hpp>
#include <utils/assert.hpp>
#include <utils/check_syscall.hpp>

namespace fs::blocking {

namespace {

int ToNative(OpenMode flags) {
  int result = 0;

  if ((flags & OpenFlag::kRead) && (flags & OpenFlag::kWrite)) {
    result |= O_RDWR;
  } else if (flags & OpenFlag::kRead) {
    result |= O_RDONLY;
  } else if (flags & OpenFlag::kWrite) {
    result |= O_WRONLY;
  } else {
    YTX_INVARIANT(false, "Specify at least one of kRead, kWrite in OpenFlags");
  }

  if (flags & OpenFlag::kCreateIfNotExists) {
    YTX_INVARIANT(flags & OpenFlag::kWrite,
                  "Cannot use kCreateIfNotExists without kWrite in OpenFlags");
    result |= O_CREAT;
  }

  if (flags & OpenFlag::kExclusiveCreate) {
    YTX_INVARIANT(flags & OpenFlag::kWrite,
                  "Cannot use kCreateIfNotExists without kWrite in OpenFlags");
    result |= O_CREAT | O_EXCL;
  }

  if (flags & OpenFlag::kTruncate) {
    YTX_INVARIANT(flags & OpenFlag::kWrite,
                  "Cannot use kTruncate without kWrite in OpenFlags");
    YTX_INVARIANT(!(flags & OpenFlag::kExclusiveCreate),
                  "Cannot use kTruncate with kExclusiveCreate in OpenFlags");
    result |= O_TRUNC;
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
  const auto fd =
      utils::CheckSyscall(::open(path.c_str(), ToNative(flags), perms),
                          "opening file '", path, "'");
  return FileDescriptor{fd};
}

FileDescriptor FileDescriptor::OpenDirectory(const std::string& path) {
  UASSERT(!path.empty());
  const auto fd =
      utils::CheckSyscall(::open(path.c_str(), O_RDONLY | O_DIRECTORY),
                          "opening directory '", path, "'");
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
  utils::CheckSyscall(::close(std::exchange(fd_, kNoFd)), "calling ::close");
}

int FileDescriptor::GetNative() const { return fd_; }

int FileDescriptor::Release() && { return std::exchange(fd_, kNoFd); }

void FileDescriptor::Write(std::string_view contents) {
  const char* buffer = contents.data();
  size_t len = contents.size();

  while (len > 0) {
    ssize_t s = ::write(fd_, buffer, len);
    if (s < 0) {
      if (errno == EAGAIN || errno == EINTR) continue;

      const auto code = std::make_error_code(std::errc(errno));
      throw std::system_error(code, "calling ::write");
    }

    buffer += s;
    len -= s;
  }
}

std::size_t FileDescriptor::Read(char* buffer, std::size_t max_size) {
  while (true) {
    const ::ssize_t s = ::read(fd_, buffer, max_size);
    if (s < 0) {
      if (errno == EAGAIN || errno == EINTR) continue;

      const auto code = std::make_error_code(std::errc(errno));
      throw std::system_error(code, "calling ::read");
    }
    return s;
  }
}

void FileDescriptor::FSync() {
  utils::CheckSyscall(::fsync(fd_), "calling ::fsync");
}

std::size_t FileDescriptor::GetSize() const {
  return GetFileStats(fd_).st_size;
}

}  // namespace fs::blocking
