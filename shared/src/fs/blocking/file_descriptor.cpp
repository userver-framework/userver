#include <fs/blocking/file_descriptor.hpp>

#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <system_error>

#include <logging/log.hpp>
#include <utils/assert.hpp>
#include <utils/string_view.hpp>

namespace fs {
namespace blocking {

/*
 * Note: use functions from C library as fstream doesn't work with
 * POSIX file descriptors => it's impossible to call sync() on
 * opened fd :(
 */

namespace {

int NativeModeFromOpenMode(FileDescriptor::OpenFlags flags) {
  return flags.GetValue();
}

}  // namespace

FileDescriptor::FileDescriptor(int fd, std::string path)
    : fd_{fd}, path_{std::move(path)} {}

FileDescriptor::FileDescriptor(FileDescriptor&& other) noexcept
    : fd_{other.fd_}, path_{std::move(other.path_)} {
  other.fd_ = -1;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept {
  std::swap(fd_, other.fd_);
  std::swap(path_, other.path_);

  return *this;
}

FileDescriptor::~FileDescriptor() {
  if (fd_ != -1) {
    try {
      Close();
    } catch (const std::exception& e) {
      LOG_ERROR() << e;
    }
  }
}

auto FileDescriptor::GetFileStats() const {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  struct ::stat result;
  if (0 != ::fstat(fd_, &result)) {
    const auto code = std::make_error_code(std::errc(errno));
    throw std::system_error(
        code, "Error while getting file stats for file '" + GetPath() + "'");
  }

  return result;
}

FileDescriptor FileDescriptor::CreateTempFile(std::string pattern) {
  UASSERT(!pattern.empty());

  constexpr char kPathPattern[] = ".XXXXXX";
  pattern += '/';
  pattern += kPathPattern;

  const auto fd = ::mkstemp(&pattern[0]);
  if (fd == -1) {
    const auto code = std::make_error_code(std::errc(errno));
    throw std::system_error(
        code,
        "Error while creating a unqiue file by pattern '" + pattern + "'");
  }

  return FileDescriptor{fd, std::move(pattern)};
}

FileDescriptor FileDescriptor::OpenFile(std::string filename, OpenFlags flags,
                                        int mode) {
  UASSERT(!filename.empty());
  const auto fd = ::open(filename.c_str(), NativeModeFromOpenMode(flags), mode);
  return FromFdChecked(fd, std::move(filename));
}

FileDescriptor FileDescriptor::OpenDirectory(std::string directory,
                                             OpenFlags flags) {
  UASSERT(!directory.empty());
  const auto directory_fd =
      ::open(directory.c_str(), NativeModeFromOpenMode(flags) | O_DIRECTORY);
  return FromFdChecked(directory_fd, std::move(directory));
}

void FileDescriptor::FSync() {
  if (::fsync(fd_) == -1) {
    const auto code = std::make_error_code(std::errc(errno));
    throw std::system_error(
        code, "Failed to fsync a descriptor for '" + GetPath() + "'");
  }
}

void FileDescriptor::Close() {
  const auto close_result = ::close(fd_);
  fd_ = -1;
  if (close_result == -1) {
    const auto code = std::make_error_code(std::errc(errno));
    throw std::system_error(
        code, "Failed to close a descriptor for '" + GetPath() + "'");
  }
}

void FileDescriptor::Write(std::string_view contents) {
  const char* buffer = contents.data();
  size_t len = contents.size();
  while (len > 0) {
    ssize_t s = write(fd_, buffer, len);
    if (s < 0) {
      if (errno == EAGAIN || errno == EINTR) continue;

      const auto code = std::make_error_code(std::errc(errno));
      throw std::system_error(
          code, "Failed to write to a descriptor for '" + GetPath() + "'");
    }

    buffer += s;
    len -= s;
  }
}

std::string FileDescriptor::ReadContents() {
  std::string result;
  for (;;) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    std::array<char, 4096> buffer;
    ssize_t s = read(fd_, buffer.data(), buffer.size());
    if (s < 0) {
      if (errno == EAGAIN || errno == EINTR) continue;

      const auto code = std::make_error_code(std::errc(errno));
      throw std::system_error(
          code, "Failed to read from a descriptor for '" + GetPath() + "'");
    } else if (s == 0) {
      break;
    }

    result.append(buffer.data(), s);
  }

  return result;
}

std::size_t FileDescriptor::GetSize() const { return GetFileStats().st_size; }

FileDescriptor FileDescriptor::FromFdChecked(int fd, std::string filename) {
  if (fd == -1) {
    const auto code = std::make_error_code(std::errc(errno));
    throw std::system_error(code,
                            "Error while opening a file '" + filename + "'");
  }

  return FileDescriptor{fd, std::move(filename)};
}

}  // namespace blocking
}  // namespace fs
