#pragma once

#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <type_traits>

#include <fmt/format.h>

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

    constexpr auto kOpenFlags = fs::blocking::OpenMode{
        fs::blocking::OpenFlag::kWrite,
        fs::blocking::OpenFlag::kCreateIfNotExists,
        fs::blocking::OpenFlag::kAppend
    };

    constexpr auto kOpenFlagsTruncate = fs::blocking::OpenMode{
        fs::blocking::OpenFlag::kWrite,
        fs::blocking::OpenFlag::kCreateIfNotExists,
        fs::blocking::OpenFlag::kTruncate
    };

    const auto flags = mode == ReopenMode::kTruncate ? kOpenFlagsTruncate : kOpenFlags;

    namespace fs = std::filesystem;
    constexpr auto kFilePermissions =
        fs::perms::owner_write | fs::perms::owner_read | fs::perms::group_read | fs::perms::others_read;
    std::string error_message{};

    for (auto tries = 0; tries < kMaxTries; ++tries) {
        try {
            std::error_code error;
            fs::create_directories(fs::path(filename).parent_path());
            fs::permissions(
                fs::path(filename).parent_path(),
                fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec | fs::perms::others_read |
                    fs::perms::others_exec,
                error
            );
            if constexpr (std::is_constructible_v<T, decltype(filename), decltype(flags), decltype(kFilePermissions)>) {
                return T(filename, flags, kFilePermissions);
            } else {
                return T::Open(filename, flags, kFilePermissions);
            }
        } catch (std::exception& e) {
            error_message = e.what() != nullptr ? e.what() : "Unknown error";
            if (engine::current_task::IsTaskProcessorThread()) {
                engine::SleepFor(kIntervalForRetries);
            } else {
                std::this_thread::sleep_for(kIntervalForRetries);
            }
        }
    }
    throw std::runtime_error(fmt::format("Filename {} cannot be created or opened: {}", filename, error_message));
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
