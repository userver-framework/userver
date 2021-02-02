#include <cache/dump/operations_file.hpp>

#include <stdexcept>
#include <utility>

#include <fmt/format.h>

#include <engine/sleep.hpp>
#include <fs/blocking/write.hpp>
#include <logging/log.hpp>

namespace cache::dump {

namespace {

constexpr std::size_t kCheckTimeAfterBytes{32 * 1024};
constexpr std::chrono::milliseconds kYieldInterval{3};

}  // namespace

FileWriter::FileWriter(std::string path, boost::filesystem::perms perms)
    : final_path_(std::move(path)),
      path_(final_path_ + ".tmp"),
      perms_(perms),
      bytes_since_last_time_check_(0),
      last_yield_time_(std::chrono::steady_clock::now()) {
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

  bytes_since_last_time_check_ += data.size();
  if (bytes_since_last_time_check_ >= kCheckTimeAfterBytes) {
    bytes_since_last_time_check_ = 0;

    const auto now = std::chrono::steady_clock::now();

    if (now - last_yield_time_ > kYieldInterval) {
      LOG_TRACE() << "cache::dump::FileWriter: yielding after using "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(
                         now - last_yield_time_)
                  << " of CPU time";

      last_yield_time_ = now;
      engine::Yield();
    }
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

std::uint64_t FileWriter::GetPosition() const {
  try {
    return file_.GetPosition();
  } catch (const std::exception& ex) {
    throw Error(
        fmt::format("Failed to fetch the written size of cache dump \"{}\": {}",
                    path_, ex.what()));
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
    const auto file_size = file_.GetSize();
    throw Error(
        fmt::format("Unexpected end-of-file while trying to read from the dump "
                    "file \"{}\": file-size={}, position={}, requested-size={}",
                    path_, file_size, file_size - bytes_read, size));
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
    throw Error(fmt::format("Failed to finalize cache dump \"{}\": {}", path_,
                            ex.what()));
  }
}

}  // namespace cache::dump
