#include <fs/blocking/temp_directory.hpp>

#include <string_view>
#include <utility>

#include <fmt/format.h>
#include <boost/filesystem/operations.hpp>

#include <fs/blocking/write.hpp>
#include <logging/log.hpp>
#include <utils/algo.hpp>
#include <utils/check_syscall.hpp>

namespace {

void RemoveDirectory(const std::string& path) {
  boost::system::error_code code;
  boost::filesystem::remove_all(path, code);
  if (code) {
    throw std::system_error(
        std::make_error_code(static_cast<std::errc>(code.value())),
        fmt::format("Failed to remove directory \"{}\"", path));
  }
}

}  // namespace

namespace fs::blocking {

TempDirectory::TempDirectory()
    : TempDirectory(
          boost::filesystem::temp_directory_path().string() + "/userver", {}) {}

TempDirectory::TempDirectory(std::string_view parent_path,
                             std::string_view name_prefix)
    : path_(utils::StrCat(parent_path, "/", name_prefix, "XXXXXX")) {
  fs::blocking::CreateDirectories(parent_path,
                                  boost::filesystem::perms::owner_all);
  utils::CheckSyscallNotEquals(::mkdtemp(path_.data()), nullptr, "::mkdtemp");
}

const std::string& TempDirectory::GetPath() const { return path_; }

void TempDirectory::Remove() && {
  const std::string path = std::move(path_);
  path_.clear();

  if (path.empty()) {
    throw std::runtime_error(
        "Remove called for an already removed TempDirectory");
  }

  RemoveDirectory(path);
}

TempDirectory::~TempDirectory() {
  if (path_.empty()) return;

  try {
    RemoveDirectory(path_);
  } catch (const std::exception& ex) {
    LOG_ERROR() << ex;
  }
}

TempDirectory::TempDirectory(TempDirectory&& other) noexcept
    : path_(std::move(other.path_)) {
  other.path_.clear();
}

TempDirectory& TempDirectory::operator=(TempDirectory&& other) noexcept {
  if (&other != this) {
    TempDirectory temp = std::move(*this);
    path_ = std::move(other.path_);
    other.path_.clear();
  }
  return *this;
}

}  // namespace fs::blocking
