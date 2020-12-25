#include <cache/dump/operations.hpp>

#include <stdexcept>
#include <utility>

#include <fmt/format.h>

#include <fs/blocking/c_file.hpp>
#include <fs/blocking/write.hpp>

namespace cache::dump {

Error::Error(std::string message) : std::runtime_error(message) {}

struct WriterImpl final {
  fs::blocking::CFile file;
  std::string path;
  std::string final_path;
};

Writer::Writer(std::string path) : impl_() {
  using Flag = fs::blocking::OpenFlag;
  impl_->path = path + ".tmp";
  impl_->final_path = std::move(path);

  try {
    impl_->file = fs::blocking::CFile{
        impl_->path, {Flag::kWrite, Flag::kExclusiveCreate}, 0600};
  } catch (const std::runtime_error& ex) {
    throw Error(fmt::format("Failed to open the dump file for write \"{}\": {}",
                            impl_->path, ex.what()));
  }
}

void Writer::WriteRaw(std::string_view data) {
  try {
    impl_->file.Write(data);
  } catch (const std::runtime_error& ex) {
    throw Error(fmt::format("Failed to write to the dump file \"{}\": {}",
                            impl_->path, ex.what()));
  }
}

void Writer::Finish() {
  try {
    impl_->file.Flush();
    std::move(impl_->file).Close();
    fs::blocking::Chmod(impl_->path, boost::filesystem::perms::owner_read);
    fs::blocking::Rename(impl_->path, impl_->final_path);
    fs::blocking::SyncDirectoryContents(
        boost::filesystem::path(impl_->final_path).parent_path().string());
  } catch (const std::runtime_error& ex) {
    throw Error(fmt::format("Failed to finalize cache dump \"{}\": {}",
                            impl_->path, ex.what()));
  }
}

Writer::~Writer() = default;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Writer::Writer(Writer&&) noexcept = default;

Writer& Writer::operator=(Writer&&) noexcept = default;

struct ReaderImpl {
  fs::blocking::CFile file;
  std::string path;
  std::string curr_chunk;
};

Reader::Reader(std::string path) : impl_() {
  using Flag = fs::blocking::OpenFlag;
  impl_->path = std::move(path);

  try {
    impl_->file = fs::blocking::CFile(impl_->path, Flag::kRead);
  } catch (const std::runtime_error& ex) {
    throw Error(fmt::format("Failed to open the dump file for read \"{}\": {}",
                            impl_->path, ex.what()));
  }
}

std::string_view Reader::ReadRaw(std::size_t size) {
  // the storage of curr_chunk_ is reused between ReadRaw calls. It acts
  // as a buffer, with its size being the capacity of the buffer.
  if (impl_->curr_chunk.size() < size) {
    impl_->curr_chunk.resize(size);
  }

  std::size_t bytes_read;
  try {
    bytes_read = impl_->file.Read(impl_->curr_chunk.data(), size);
  } catch (const std::runtime_error& ex) {
    throw Error(fmt::format("Failed to read from the dump file \"{}\": {}",
                            impl_->path, ex.what()));
  }

  if (bytes_read != size) {
    throw Error(fmt::format(
        "Sudden end-of-file while trying to read from the dump file \"{}\"",
        impl_->path));
  }

  return std::string_view(impl_->curr_chunk.data(), size);
}

void Reader::Finish() {
  std::size_t bytes_read;

  try {
    char extra_byte{};
    bytes_read = impl_->file.Read(&extra_byte, 1);
  } catch (const std::runtime_error& ex) {
    throw Error(fmt::format("Failed to read from the dump file \"{}\": {}",
                            impl_->path, ex.what()));
  }

  if (bytes_read != 0) {
    throw Error(
        fmt::format("There is unread data at the end of the dump file \"{}\"",
                    impl_->path));
  }

  try {
    std::move(impl_->file).Close();
  } catch (const std::runtime_error& ex) {
    throw Error(fmt::format("Failed to finalize cache dump \"{}\": {}",
                            impl_->path, ex.what()));
  }
}

Reader::~Reader() = default;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Reader::Reader(Reader&&) noexcept = default;

Reader& Reader::operator=(Reader&&) noexcept = default;

}  // namespace cache::dump
