#pragma once

#include <chrono>

#include <boost/filesystem/operations.hpp>

#include <fs/blocking/c_file.hpp>
#include <utils/fast_pimpl.hpp>

#include <cache/dump/operations.hpp>

namespace cache::dump {

/// A handle to a cache dump file. File operations block the thread.
class FileWriter final : public Writer {
 public:
  /// @brief Creates a new dump file and opens it
  /// @throws `Error` on a filesystem error
  explicit FileWriter(std::string path, boost::filesystem::perms perms);

  /// @brief Must be called once all data has been written
  /// @throws `Error` on a filesystem error
  void Finish();

  /// @brief Get the size of data that has been written so far
  /// @warning Can only be called before `Finish`
  /// @throws `Error` on a filesystem error
  std::uint64_t GetPosition() const;

  void WriteRaw(std::string_view data) override;
  using Writer::Write;

 private:
  fs::blocking::CFile file_;
  std::string final_path_;
  std::string path_;
  boost::filesystem::perms perms_;
  std::size_t bytes_since_last_time_check_;
  std::chrono::steady_clock::time_point last_yield_time_;
};

/// A handle to a cache dump file. File operations block the thread.
class FileReader final : public Reader {
 public:
  /// @brief Opens an existing dump file
  /// @throws `Error` on a filesystem error
  explicit FileReader(std::string path);

  /// @brief Must be called once all data has been read
  /// @throws `Error` on a filesystem error or if there is leftover data
  void Finish();

  std::string_view ReadRaw(std::size_t size) override;
  using Reader::Read;

 private:
  fs::blocking::CFile file_;
  std::string path_;
  std::string curr_chunk_;
};

}  // namespace cache::dump
