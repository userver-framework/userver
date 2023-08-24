#pragma once

#include <thread>

#include <fmt/format.h>
#include <boost/filesystem.hpp>

#include <logging/impl/reopen_mode.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_base.hpp>
#include <userver/fs/blocking/c_file.hpp>
#include <userver/fs/blocking/file_descriptor.hpp>
#include <userver/fs/blocking/open_mode.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

template <class T>
T OpenFile(const std::string& filename, ReopenMode mode = ReopenMode::kAppend) {
  // count tries for open file
  constexpr int kMaxTries = 5;
  constexpr std::chrono::milliseconds kIntervalForRetries{10};

  constexpr auto kOpenFlags =
      fs::blocking::OpenMode{fs::blocking::OpenFlag::kWrite,
                             fs::blocking::OpenFlag::kCreateIfNotExists,
                             fs::blocking::OpenFlag::kAppend};

  constexpr auto kOpenFlagsTruncate =
      fs::blocking::OpenMode{fs::blocking::OpenFlag::kWrite,
                             fs::blocking::OpenFlag::kCreateIfNotExists,
                             fs::blocking::OpenFlag::kTruncate};

  const auto flags =
      mode == ReopenMode::kTruncate ? kOpenFlagsTruncate : kOpenFlags;

  namespace fs = boost::filesystem;
  constexpr auto kFilePermissions =
      fs::owner_write | fs::owner_read | fs::group_read | fs::others_read;

  for (auto tries = 0; tries < kMaxTries; ++tries) {
    try {
      boost::system::error_code error;
      fs::create_directories(fs::path(filename).parent_path());
      fs::permissions(fs::path(filename).parent_path(),
                      fs::owner_all | fs::group_read | fs::group_exe |
                          fs::others_read | fs::others_exe,
                      error);
      if constexpr (std::is_constructible_v<T, decltype(filename),
                                            decltype(flags),
                                            decltype(kFilePermissions)>) {
        return T(filename, flags, kFilePermissions);
      } else {
        return T::Open(filename, flags, kFilePermissions);
      }
    } catch (std::exception&) {
      if (engine::current_task::IsTaskProcessorThread()) {
        engine::SleepFor(kIntervalForRetries);
      } else {
        std::this_thread::sleep_for(kIntervalForRetries);
      }
    }
  }
  throw std::runtime_error(
      fmt::format("Filename {} cannot be created or opened", filename));
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
