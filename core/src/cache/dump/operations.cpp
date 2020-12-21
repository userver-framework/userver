#include <cache/dump/operations.hpp>

#include <cstdio>
#include <limits>
#include <memory>
#include <system_error>
#include <utility>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fmt/format.h>

#include <fs/blocking/write.hpp>
#include <logging/log.hpp>
#include <utils/check_syscall.hpp>

namespace cache::dump {

namespace {

std::string ErrnoMessage(int code) {
  return std::error_code(code, std::generic_category()).message();
}

struct FileDeleter {
  void operator()(std::FILE* file) noexcept {
    // unique_ptr guarantees that file != nullptr
    if (std::fclose(file) == -1) {
      LOG_ERROR() << "Error while closing a cache dump file: "
                  << ErrnoMessage(errno);
    }
  }
};

using FileHolder = std::unique_ptr<std::FILE, FileDeleter>;

}  // namespace

Error::Error(std::string message) : std::runtime_error(message) {}

struct WriterImpl final {
  std::string path;
  std::string tmp_path;
  FileHolder file;
};

Writer::Writer(std::string path) : impl_() {
  impl_->path = std::move(path);
  impl_->tmp_path = impl_->path + ".tmp";

  try {
    const int fd = utils::CheckSyscall(
        ::open(impl_->tmp_path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600),
        "open");
    impl_->file.reset(
        utils::CheckSyscallNotEquals(::fdopen(fd, "wbm"), nullptr, "fdopen"));
  } catch (const std::exception& ex) {
    throw Error(fmt::format("Failed to open the dump file for write \"{}\": {}",
                            impl_->path, ex.what()));
  }
}

void Writer::WriteRaw(std::string_view data) {
  if (data.empty()) return;

  if (std::fwrite(data.data(), data.size(), 1, impl_->file.get()) != 1) {
    throw Error(fmt::format("Failed to write to the dump file \"{}\": {}",
                            impl_->path,
                            ErrnoMessage(std::ferror(impl_->file.get()))));
  }
}

void Writer::Finish() {
  try {
    utils::CheckSyscall(std::fflush(impl_->file.get()), "fflush");

    const int fd = utils::CheckSyscall(::fileno(impl_->file.get()), "fileno");
    utils::CheckSyscall(::fsync(fd), "fsync");
    utils::CheckSyscall(::fchmod(fd, 0400), "chmod");

    utils::CheckSyscall(std::fclose(impl_->file.release()), "fclose");

    fs::blocking::Rename(impl_->tmp_path, impl_->path);

    const auto directory = boost::filesystem::path(impl_->path).parent_path();
    fs::blocking::SyncDirectoryContents(directory.string());
  } catch (const std::exception& ex) {
    throw Error(fmt::format("Failed to finalize cache dump \"{}\": {}",
                            impl_->path, ex.what()));
  }
}

Writer::~Writer() = default;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Writer::Writer(Writer&&) noexcept = default;

Writer& Writer::operator=(Writer&&) noexcept = default;

struct ReaderImpl {
  std::string path;
  FileHolder file;
  std::string curr_chunk;
};

Reader::Reader(std::string path) : impl_() {
  impl_->path = std::move(path);

  try {
    impl_->file.reset(utils::CheckSyscallNotEquals(
        std::fopen(impl_->path.c_str(), "rbm"), nullptr, "fopen"));
  } catch (const std::exception& ex) {
    throw Error(fmt::format("Failed to open the dump file for read \"{}\": {}",
                            impl_->path, ex.what()));
  }
}

std::string_view Reader::ReadRaw(std::size_t size) {
  if (size == 0) return {};

  // the storage of curr_chunk_ is reused between ReadRaw calls. It acts
  // as a buffer, with its size being the capacity of the buffer.
  if (impl_->curr_chunk.size() < size) {
    impl_->curr_chunk.resize(size);
  }

  if (std::fread(impl_->curr_chunk.data(), size, 1, impl_->file.get()) != 1) {
    if (std::feof(impl_->file.get())) {
      throw Error(fmt::format(
          "Sudden end-of-file while trying to read from the dump file \"{}\"",
          impl_->path));
    } else {
      throw Error(fmt::format("Failed to read from the dump file \"{}\": {}",
                              impl_->path,
                              ErrnoMessage(std::ferror(impl_->file.get()))));
    }
  }

  return std::string_view(impl_->curr_chunk.data(), size);
}

void Reader::Finish() {
  char extra_byte{};
  const auto bytes_read =
      std::fread(&extra_byte, sizeof(extra_byte), 1, impl_->file.get());

  const auto error = std::ferror(impl_->file.get());
  if (error != 0) {
    throw Error(fmt::format("Failed to read from the dump file \"{}\": {}",
                            impl_->path, ErrnoMessage(error)));
  }

  if (bytes_read != 0 || std::feof(impl_->file.get()) == 0) {
    throw Error(
        fmt::format("There is unread data at the end of the dump file \"{}\"",
                    impl_->path));
  }

  try {
    utils::CheckSyscall(std::fclose(impl_->file.release()), "fclose");
  } catch (const std::exception& ex) {
    throw Error(fmt::format("Failed to finalize cache dump \"{}\": {}",
                            impl_->path, ex.what()));
  }
}

Reader::~Reader() = default;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Reader::Reader(Reader&&) noexcept = default;

Reader& Reader::operator=(Reader&&) noexcept = default;

}  // namespace cache::dump
