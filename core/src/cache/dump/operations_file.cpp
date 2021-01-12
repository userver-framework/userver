#include <cache/dump/operations_file.hpp>

#include <stdexcept>
#include <utility>

#include <fmt/format.h>

#include <fs/blocking/write.hpp>

namespace cache::dump {

FileWriter::FileWriter(std::string path, boost::filesystem::perms perms)
    : final_path_(std::move(path)), path_(final_path_ + ".tmp"), perms_(perms) {
  constexpr fs::blocking::OpenMode mode{
      fs::blocking::OpenFlag::kWrite, fs::blocking::OpenFlag::kExclusiveCreate};
  const int tmp_perms =
      static_cast<int>(perms_ | boost::filesystem::perms::owner_write);

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
    throw Error(fmt::format("Failed to finalize cache dump \"{}\": {}", path_,
                            ex.what()));
  }
}

FileReader::FileReader(std::string path) : path_(std::move(path)) {
  try {
    file_ = fs::blocking::CFile(path_, fs::blocking::OpenFlag::kRead);
  } catch (const std::exception& ex) {
    throw Error(fmt::format("Failed to open the dump file for read \"{}\": {}",
                            path_, ex.what()));
  }
}

std::string_view FileReader::ReadRaw(std::size_t size) {
  // the storage of curr_chunk_ is reused between ReadRaw calls. It acts
  // as a buffer, with its size being the capacity of the buffer.
  if (curr_chunk_.size() < size) {
    curr_chunk_.resize(size);
  }

  std::size_t bytes_read;
  try {
    bytes_read = file_.Read(curr_chunk_.data(), size);
  } catch (const std::exception& ex) {
    throw Error(fmt::format("Failed to read from the dump file \"{}\": {}",
                            path_, ex.what()));
  }

  if (bytes_read != size) {
    throw Error(fmt::format(
        "Sudden end-of-file while trying to read from the dump file \"{}\"",
        path_));
  }

  return std::string_view(curr_chunk_.data(), size);
}

void FileReader::Finish() {
  std::size_t bytes_read;

  try {
    char extra_byte{};
    bytes_read = file_.Read(&extra_byte, 1);
  } catch (const std::exception& ex) {
    throw Error(fmt::format("Failed to read from the dump file \"{}\": {}",
                            path_, ex.what()));
  }

  if (bytes_read != 0) {
    throw Error(fmt::format(
        "There is unread data at the end of the dump file \"{}\"", path_));
  }

  try {
    std::move(file_).Close();
  } catch (const std::exception& ex) {
    throw Error(fmt::format("Failed to finalize cache dump \"{}\": {}", path_,
                            ex.what()));
  }
}

}  // namespace cache::dump
