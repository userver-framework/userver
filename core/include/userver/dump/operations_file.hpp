#pragma once

#include <chrono>

#include <boost/filesystem/operations.hpp>

#include <userver/fs/blocking/c_file.hpp>
#include <userver/utils/cpu_relax.hpp>
#include <userver/utils/fast_pimpl.hpp>

#include <userver/dump/factory.hpp>
#include <userver/dump/operations.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

/// A handle to a dump file. File operations block the thread.
class FileWriter final : public Writer {
 public:
  /// @brief Creates a new dump file and opens it
  /// @throws `Error` on a filesystem error
  explicit FileWriter(std::string path, boost::filesystem::perms perms,
                      tracing::ScopeTime& scope);

  void Finish() override;

 private:
  void WriteRaw(std::string_view data) override;

  fs::blocking::CFile file_;
  std::string final_path_;
  std::string path_;
  boost::filesystem::perms perms_;
  utils::StreamingCpuRelax cpu_relax_;
};

/// A handle to a dump file. File operations block the thread.
class FileReader final : public Reader {
 public:
  /// @brief Opens an existing dump file
  /// @throws `Error` on a filesystem error
  explicit FileReader(std::string path);

  void Finish() override;

 private:
  std::string_view ReadRaw(std::size_t max_size) override;

  fs::blocking::CFile file_;
  std::string path_;
  std::string curr_chunk_;
};

class FileOperationsFactory final : public OperationsFactory {
 public:
  explicit FileOperationsFactory(boost::filesystem::perms perms);

  std::unique_ptr<Reader> CreateReader(std::string full_path) override;

  std::unique_ptr<Writer> CreateWriter(std::string full_path,
                                       tracing::ScopeTime& scope) override;

 private:
  const boost::filesystem::perms perms_;
};

}  // namespace dump

USERVER_NAMESPACE_END
