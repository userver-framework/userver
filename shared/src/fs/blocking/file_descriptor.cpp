#include <fs/blocking/file_descriptor.hpp>

#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <array>
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
    throw std::logic_error(
        "Specify at least one of kRead, kWrite in OpenFlags");
  }

  if (flags & OpenFlag::kCreateIfNotExists) {
    if (!(flags & OpenFlag::kWrite)) {
      throw std::logic_error(
          "Cannot use kCreateIfNotExists without kWrite in OpenFlags");
    }
    result |= O_CREAT;
  }

  if (flags & OpenFlag::kExclusiveCreate) {
    if (!(flags & OpenFlag::kWrite)) {
      throw std::logic_error(
          "Cannot use kCreateIfNotExists without kWrite in OpenFlags");
    }
    result |= O_CREAT | O_EXCL;
  }

  return result;
}

constexpr int kNoFd = -1;

}  // namespace

FileDescriptor::FileDescriptor() : fd_(kNoFd) {}

FileDescriptor::FileDescriptor(int fd, std::string&& path)
    : fd_{fd}, path_{std::move(path)} {}

FileDescriptor::FileDescriptor(FileDescriptor&& other) noexcept
    : fd_{other.fd_}, path_{std::move(other.path_)} {
  other.fd_ = kNoFd;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept {
  std::swap(fd_, other.fd_);
  std::swap(path_, other.path_);

  return *this;
}

FileDescriptor::~FileDescriptor() {
  if (fd_ != kNoFd) {
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
  utils::CheckSyscall(::fstat(fd_, &result), "getting file stats for file '",
                      GetPath(), "'");
  return result;
}

FileDescriptor FileDescriptor::CreateTempFile() {
  return CreateTempFile(boost::filesystem::temp_directory_path().string() +
                        "/userver");
}

FileDescriptor FileDescriptor::CreateTempFile(std::string pattern) {
  UASSERT(!pattern.empty());

  constexpr std::string_view kPathSuffix = "/XXXXXX";
  pattern += kPathSuffix;

  const int fd = utils::CheckSyscall(
      ::mkstemp(&pattern[0]),
      "creating a unique file by pattern '" + pattern + "'");

  return FileDescriptor{fd, std::move(pattern)};
}

FileDescriptor FileDescriptor::OpenFile(std::string filename, OpenMode flags,
                                        int perms) {
  UASSERT(!filename.empty());
  const auto fd =
      utils::CheckSyscall(::open(filename.c_str(), ToNative(flags), perms),
                          "opening file '" + filename + "'");
  return FileDescriptor{fd, std::move(filename)};
}

FileDescriptor FileDescriptor::OpenDirectory(std::string directory,
                                             OpenMode flags) {
  UASSERT(!directory.empty());
  const auto fd = utils::CheckSyscall(
      ::open(directory.c_str(), ToNative(flags) | O_DIRECTORY),
      "opening directory '" + directory + "'");
  return FileDescriptor{fd, std::move(directory)};
}

void FileDescriptor::FSync() {
  utils::CheckSyscall(::fsync(fd_), "fsync-ing a descriptor for '", GetPath(),
                      "'");
}

void FileDescriptor::Close() {
  utils::CheckSyscall(::close(std::exchange(fd_, -1)),
                      "closing a descriptor for '", GetPath(), "'");
}

void FileDescriptor::Write(std::string_view contents) {
  const char* buffer = contents.data();
  size_t len = contents.size();
  while (len > 0) {
    ssize_t s = ::write(fd_, buffer, len);
    if (s < 0) {
      if (errno == EAGAIN || errno == EINTR) continue;

      const auto code = std::make_error_code(std::errc(errno));
      throw std::system_error(
          code, "writing to a descriptor for '" + GetPath() + "'");
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
    const ssize_t s = ::read(fd_, buffer.data(), buffer.size());
    if (s < 0) {
      if (errno == EAGAIN || errno == EINTR) continue;

      const auto code = std::make_error_code(std::errc(errno));
      throw std::system_error(
          code, "reading from a descriptor for '" + GetPath() + "'");
    } else if (s == 0) {
      break;
    }

    result.append(buffer.data(), s);
  }

  return result;
}

int FileDescriptor::Release() && { return std::exchange(fd_, kNoFd); }

std::size_t FileDescriptor::GetSize() const { return GetFileStats().st_size; }

}  // namespace fs::blocking
