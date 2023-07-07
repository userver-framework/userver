#include <userver/fs/blocking/temp_file.hpp>

#include <utility>

#include <boost/filesystem/operations.hpp>

#include <userver/fs/blocking/file_descriptor.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <utils/check_syscall.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs::blocking {

TempFile TempFile::Create() {
  return Create(boost::filesystem::temp_directory_path().string(), "userver-");
}

TempFile TempFile::Create(std::string_view parent_path,
                          std::string_view name_prefix) {
  UASSERT(!parent_path.empty());
  CreateDirectories(parent_path, boost::filesystem::perms::owner_all);
  auto path = utils::StrCat(parent_path, "/", name_prefix, "XXXXXX");
  FileDescriptor{utils::CheckSyscall(::mkstemp(path.data()),
                                     "creating a unique file by pattern '{}'",
                                     path)}
      .Close();
  LOG_DEBUG() << "Created a temporary file: " << path;
  return TempFile{std::move(path)};
}

TempFile::TempFile(std::string&& path) : path_(std::move(path)) {
  UASSERT(boost::filesystem::exists(path_));
  UASSERT(!boost::filesystem::is_directory(path_));
}

TempFile::TempFile(TempFile&& other) noexcept
    : path_(std::exchange(other.path_, {})) {}

TempFile& TempFile::operator=(TempFile&& other) noexcept {
  if (&other != this) {
    const TempFile temp = std::move(*this);
    path_ = std::exchange(other.path_, {});
  }
  return *this;
}

TempFile::~TempFile() {
  if (path_.empty()) return;

  try {
    fs::blocking::RemoveSingleFile(path_);
  } catch (const std::exception& ex) {
    LOG_ERROR() << ex;
  }
}

TempFile TempFile::Adopt(std::string path) { return TempFile{std::move(path)}; }

const std::string& TempFile::GetPath() const { return path_; }

void TempFile::Remove() && {
  if (path_.empty()) {
    throw std::runtime_error("Remove called for an already removed TempFile");
  }

  fs::blocking::RemoveSingleFile(std::exchange(path_, {}));
}

}  // namespace fs::blocking

USERVER_NAMESPACE_END
