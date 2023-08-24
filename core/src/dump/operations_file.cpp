#include <userver/dump/operations_file.hpp>

#include <algorithm>
#include <stdexcept>
#include <utility>

#include <fmt/format.h>

#include <userver/fs/blocking/write.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

namespace {
constexpr std::size_t kCheckTimeAfterBytes{1 << 15};
}

FileWriter::FileWriter(std::string path, boost::filesystem::perms perms,
                       tracing::ScopeTime& scope)
    : final_path_(std::move(path)),
      path_(final_path_ + ".tmp"),
      perms_(perms),
      cpu_relax_(kCheckTimeAfterBytes, &scope) {
  constexpr fs::blocking::OpenMode mode{
      fs::blocking::OpenFlag::kWrite, fs::blocking::OpenFlag::kExclusiveCreate};
  const auto tmp_perms = perms_ | boost::filesystem::perms::owner_write;

  try {
    file_ = fs::blocking::CFile{path_, mode, tmp_perms};
  } catch (const std::exception& ex) {
    throw Error(fmt::format("Failed to open the dump file for write \"{}\": {}",
                            path_, ex.what()));
  }
}

void FileWriter::WriteRaw(std::string_view data) {
  try {
    file_.Write(data);
  } catch (const std::exception& ex) {
    throw Error(fmt::format("Failed to write to the dump file \"{}\": {}",
                            path_, ex.what()));
  }
  cpu_relax_.Relax(data.size());
}

void FileWriter::Finish() {
  try {
    file_.Flush();
    std::move(file_).Close();
    fs::blocking::Chmod(path_, perms_);  // drop perms::owner_write
    fs::blocking::Rename(path_, final_path_);
    fs::blocking::SyncDirectoryContents(
        boost::filesystem::path(final_path_).parent_path().string());
  } catch (const std::exception& ex) {
    throw Error(fmt::format("Failed to finalize dump \"{}\". Reason: {}", path_,
                            ex.what()));
  }
}

FileReader::FileReader(std::string path) : path_(std::move(path)) {
  try {
    file_ = fs::blocking::CFile(path_, fs::blocking::OpenFlag::kRead);
  } catch (const std::exception& ex) {
    throw Error(fmt::format(
        "Failed to open the dump file for reading \"{}\". Reason: {}", path_,
        ex.what()));
  }
}

std::string_view FileReader::ReadRaw(std::size_t max_size) {
  // the storage of curr_chunk_ is reused between ReadRaw calls. It acts
  // as a buffer, with its size being the capacity of the buffer.
  if (curr_chunk_.size() < max_size) {
    curr_chunk_.resize(
        std::max(max_size, static_cast<std::size_t>(curr_chunk_.size() * 1.5)));
  }

  std::size_t bytes_read = 0;
  try {
    bytes_read = file_.Read(curr_chunk_.data(), max_size);
  } catch (const std::exception& ex) {
    throw Error(fmt::format("Failed to read from the dump file \"{}\": {}",
                            path_, ex.what()));
  }

  return {curr_chunk_.data(), bytes_read};
}

void FileReader::Finish() {
  std::size_t bytes_read = 0;

  try {
    char extra_byte = 0;
    bytes_read = file_.Read(&extra_byte, 1);
  } catch (const std::exception& ex) {
    throw Error(fmt::format("Failed to read from the dump file \"{}\": {}",
                            path_, ex.what()));
  }

  if (bytes_read != 0) {
    const auto file_size = file_.GetSize();
    const auto position = file_.GetPosition() - bytes_read;
    throw Error(
        fmt::format("Unexpected extra data at the end of the dump file \"{}\": "
                    "file-size={}, position={}, unread-size={}",
                    path_, file_size, position, file_size - position));
  }

  try {
    std::move(file_).Close();
  } catch (const std::exception& ex) {
    throw Error(fmt::format("Failed to finalize dump file \"{}\". Reason: {}",
                            path_, ex.what()));
  }
}

FileOperationsFactory::FileOperationsFactory(boost::filesystem::perms perms)
    : perms_(perms) {}

std::unique_ptr<Reader> FileOperationsFactory::CreateReader(
    std::string full_path) {
  return std::make_unique<FileReader>(std::move(full_path));
}

std::unique_ptr<Writer> FileOperationsFactory::CreateWriter(
    std::string full_path, tracing::ScopeTime& scope) {
  return std::make_unique<FileWriter>(std::move(full_path), perms_, scope);
}

}  // namespace dump

USERVER_NAMESPACE_END
