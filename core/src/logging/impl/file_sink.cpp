#include "file_sink.hpp"

#include <spdlog/details/fmt_helper.h>
#include <boost/filesystem.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

namespace {

constexpr auto kOpenFlags = fs::blocking::OpenMode{
    fs::blocking::OpenFlag::kWrite, fs::blocking::OpenFlag::kCreateIfNotExists,
    fs::blocking::OpenFlag::kAppend};

constexpr auto kOpenFlagsTruncate = fs::blocking::OpenMode{
    fs::blocking::OpenFlag::kWrite, fs::blocking::OpenFlag::kCreateIfNotExists,
    fs::blocking::OpenFlag::kTruncate};

fs::blocking::FileDescriptor OpenFile(
    const std::string& filename,
    FileSink::ReopenMode mode = FileSink::ReopenMode::kAppend) {
  // count tries for open file
  constexpr int kMaxTries = 5;
  const std::chrono::milliseconds interval_for_tries{10};

  for (auto tries = 0; tries < kMaxTries; ++tries) {
    try {
      namespace fs = boost::filesystem;
      fs::create_directories(fs::path(filename).parent_path());
      fs::permissions(fs::path(filename).parent_path(),
                      fs::owner_all | fs::group_read | fs::group_exe |
                          fs::others_read | fs::others_exe);
      return USERVER_NAMESPACE::fs::blocking::FileDescriptor::Open(
          filename,
          mode == FileSink::ReopenMode::kTruncate ? kOpenFlagsTruncate
                                                  : kOpenFlags,
          fs::owner_write | fs::owner_read | fs::group_read | fs::others_read);
    } catch (std::exception&) {
      std::this_thread::sleep_for(interval_for_tries);
    }
  }
  // return throw
  return fs::blocking::FileDescriptor::Open(
      filename, mode == FileSink::ReopenMode::kTruncate ? kOpenFlagsTruncate
                                                        : kOpenFlags);
}

}  // namespace

FileSink::FileSink(const std::string& filename)
    : FdSink(OpenFile(filename)), filename_{filename} {
  if (GetFd().GetSize() > 0) {
    GetFd().Write("\n");
  }
}

void FileSink::Reopen(FileSink::ReopenMode mode) {
  std::lock_guard lock{GetMutex()};
  GetFd().FSync();
  std::move(GetFd()).Close();
  SetFd(OpenFile(filename_, mode));
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
