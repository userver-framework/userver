#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/fs/fs_cache_client.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/mock_now.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr auto kUpdatePeriod = std::chrono::milliseconds{100};

fs::FsCacheClient::Settings UpdatingSettings(const std::string& dir) {
    return {
        .dir = dir,
        .update_period = kUpdatePeriod,
        .task_processor = engine::current_task::GetTaskProcessor(),
    };
}

void WaitUntilCached(
    const fs::FsCacheClient& cache,
    std::string_view path,
    std::optional<std::string> expected_contents
) {
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
    while (!deadline.IsReached()) {
        const auto file = cache.TryGetFile(path);
        if (!expected_contents.has_value()) {
            if (!file) {
                return;
            }
        } else if (file && std::holds_alternative<std::string>(file->data_or_path) &&
                   std::get<std::string>(file->data_or_path) == *expected_contents)
        {
            return;
        }
        utils::datetime::MockSleep(kUpdatePeriod);
        engine::SleepFor(std::chrono::milliseconds{10});
    }
    ADD_FAILURE() << "Timed out waiting for cache update of " << path;
}

// Waits until inotify / PeriodicTask is actually watching the directory.
void WaitUntilWatcherReady(const fs::FsCacheClient& cache, const std::string& dir) {
    utils::datetime::MockSleep(kUpdatePeriod);
    engine::SleepFor(kUpdatePeriod);
    fs::blocking::RewriteFileContents(dir + "/watcher-ready.txt", "ready");
    WaitUntilCached(cache, "/watcher-ready.txt", "ready");
}

}  // namespace

UTEST(FsCacheClient, MaxSizeToCacheZero) {
    const auto dir = fs::blocking::TempDirectory::Create();
    const auto file_path = dir.GetPath() + "/data.txt";
    constexpr std::string_view kContents = "not cached";
    fs::blocking::RewriteFileContents(file_path, kContents);

    const fs::FsCacheClient cache{{
        .dir = dir.GetPath(),
        .update_period = std::chrono::milliseconds{0},
        .task_processor = engine::current_task::GetTaskProcessor(),
        .max_size_to_cache = 0,
    }};

    const auto file = cache.TryGetFile("/data.txt");
    ASSERT_TRUE(file);
    ASSERT_TRUE(std::holds_alternative<std::filesystem::path>(file->data_or_path));
    EXPECT_EQ(
        std::filesystem::absolute(std::filesystem::path{file_path}),
        std::get<std::filesystem::path>(file->data_or_path)
    );
    EXPECT_EQ(file->extension, ".txt");
}

UTEST_MT(FsCacheClient, DetectsFileUpdate, 3) {
    utils::datetime::MockNowSet({});

    const auto dir = fs::blocking::TempDirectory::Create();
    const auto file_path = dir.GetPath() + "/data.txt";
    fs::blocking::RewriteFileContents(file_path, "old");

    const fs::FsCacheClient cache{UpdatingSettings(dir.GetPath())};
    WaitUntilCached(cache, "/data.txt", "old");
    WaitUntilWatcherReady(cache, dir.GetPath());

    fs::blocking::RewriteFileContents(file_path, "new");
    WaitUntilCached(cache, "/data.txt", "new");
}

UTEST_MT(FsCacheClient, DetectsNewFile, 3) {
    utils::datetime::MockNowSet({});

    const auto dir = fs::blocking::TempDirectory::Create();
    const fs::FsCacheClient cache{UpdatingSettings(dir.GetPath())};
    WaitUntilWatcherReady(cache, dir.GetPath());

    fs::blocking::RewriteFileContents(dir.GetPath() + "/created.txt", "fresh");
    WaitUntilCached(cache, "/created.txt", "fresh");
}

UTEST_MT(FsCacheClient, DetectsFileDelete, 3) {
    utils::datetime::MockNowSet({});

    const auto dir = fs::blocking::TempDirectory::Create();
    const auto file_path = dir.GetPath() + "/data.txt";
    fs::blocking::RewriteFileContents(file_path, "keep-me");

    const fs::FsCacheClient cache{UpdatingSettings(dir.GetPath())};
    WaitUntilCached(cache, "/data.txt", "keep-me");
    WaitUntilWatcherReady(cache, dir.GetPath());

    ASSERT_TRUE(fs::blocking::RemoveSingleFile(file_path));
    WaitUntilCached(cache, "/data.txt", std::nullopt);
}

USERVER_NAMESPACE_END
