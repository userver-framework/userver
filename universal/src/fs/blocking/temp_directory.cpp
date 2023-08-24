#include <userver/fs/blocking/temp_directory.hpp>

#include <utility>

#include <fmt/format.h>
#include <boost/filesystem/operations.hpp>

#include <userver/fs/blocking/write.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/algo.hpp>
#include <utils/check_syscall.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs::blocking {

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

TempDirectory TempDirectory::Create() {
  return Create(boost::filesystem::temp_directory_path().string(), "userver-");
}

TempDirectory TempDirectory::Create(std::string_view parent_path,
                                    std::string_view name_prefix) {
  CreateDirectories(parent_path, boost::filesystem::perms::owner_all);
  auto path = utils::StrCat(parent_path, "/", name_prefix, "XXXXXX");
  utils::CheckSyscallNotEquals(::mkdtemp(path.data()), nullptr, "::mkdtemp");
  LOG_DEBUG() << "Created a temporary directory: " << path;
  return TempDirectory{std::move(path)};
}

TempDirectory::TempDirectory(std::string&& path) : path_(std::move(path)) {
  UASSERT(boost::filesystem::is_directory(path_));
}

TempDirectory::TempDirectory(TempDirectory&& other) noexcept
    : path_(std::exchange(other.path_, {})) {}

TempDirectory& TempDirectory::operator=(TempDirectory&& other) noexcept {
  if (&other != this) {
    const TempDirectory temp = std::move(*this);
    path_ = std::exchange(other.path_, {});
  }
  return *this;
}

TempDirectory::~TempDirectory() {
  if (path_.empty()) return;

  try {
    RemoveDirectory(path_);
  } catch (const std::exception& ex) {
    LOG_ERROR() << ex;
  }
}

TempDirectory TempDirectory::Adopt(std::string path) {
  return TempDirectory{std::move(path)};
}

const std::string& TempDirectory::GetPath() const { return path_; }

void TempDirectory::Remove() && {
  if (path_.empty()) {
    throw std::runtime_error(
        "Remove called for an already removed TempDirectory");
  }
  RemoveDirectory(std::exchange(path_, {}));
}

}  // namespace fs::blocking

USERVER_NAMESPACE_END
