#include "file_sink.hpp"

#include <functional>

#include <boost/filesystem/operations.hpp>

#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/utest/parameter_names.hpp>
#include <userver/utest/utest.hpp>

#include "buffered_file_sink.hpp"
#include "sink_helper_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

using SinkPtr = std::unique_ptr<logging::impl::BaseSink>;

struct SinkFactory final {
    std::string test_name;
    std::function<SinkPtr(const std::string& filename)> sink_factory;
};

SinkPtr MakeFileSink(const std::string& filename) { return std::make_unique<logging::impl::FileSink>(filename); }

SinkPtr MakeBufferedFileSink(const std::string& filename) {
    return std::make_unique<logging::impl::BufferedFileSink>(filename);
}

class FileSinks : public testing::TestWithParam<SinkFactory> {
protected:
    const std::string& GetTempRootPath() const { return temp_root_.GetPath(); }

    const std::string& Filename() const { return filename_; }

    logging::impl::BaseSink& Sink() { return *sink_; }

    void LogSingleMessageAndCheck() {
        EXPECT_NO_THROW(Sink().Log({"message\n", logging::Level::kWarning}));
        EXPECT_NO_THROW(Sink().Flush());
        EXPECT_EQ(test::ReadFromFile(Filename()), test::Messages("message"));
    }

private:
    const fs::blocking::TempDirectory temp_root_ = fs::blocking::TempDirectory::Create();
    const std::string filename_ = temp_root_.GetPath() + "/temp_file";
    const SinkPtr sink_ = GetParam().sink_factory(filename_);
};

}  // namespace

UTEST_P(FileSinks, TestCreateFile) { EXPECT_EQ(test::ReadFromFile(Filename()), test::Messages()); }

UTEST_P(FileSinks, TestCreateFileMultiDir) {
    const auto subdir_filename = GetTempRootPath() + "/dir1/dir2/dir3/temp_file";
    const auto subdir_sink = GetParam().sink_factory(subdir_filename);
    EXPECT_EQ(test::ReadFromFile(subdir_filename), test::Messages());
}

UTEST_P(FileSinks, TestWriteInFile) { EXPECT_NO_THROW(Sink().Log({"message\n", logging::Level::kCritical})); }

UTEST_P(FileSinks, CheckPermissionsFile) {
    const auto stat = boost::filesystem::status(Filename());
    ASSERT_TRUE(stat.permissions() & boost::filesystem::owner_read);
    ASSERT_TRUE(stat.permissions() & boost::filesystem::owner_write);
    ASSERT_FALSE(stat.permissions() & boost::filesystem::owner_exe);
    ASSERT_TRUE(stat.permissions() & boost::filesystem::group_read);
    ASSERT_FALSE(stat.permissions() & boost::filesystem::group_write);
    ASSERT_FALSE(stat.permissions() & boost::filesystem::group_exe);
    ASSERT_TRUE(stat.permissions() & boost::filesystem::others_read);
    ASSERT_FALSE(stat.permissions() & boost::filesystem::others_write);
    ASSERT_FALSE(stat.permissions() & boost::filesystem::others_exe);
}

UTEST_P(FileSinks, CheckPermissionsDir) {
    const auto subdir_filename = GetTempRootPath() + "/dir/temp_file";
    const auto subdir_sink = GetParam().sink_factory(subdir_filename);
    EXPECT_EQ(test::ReadFromFile(subdir_filename), test::Messages());
    const auto path = boost::filesystem::path(subdir_filename).parent_path();

    const auto stat = boost::filesystem::status(path);
    ASSERT_TRUE(stat.permissions() & boost::filesystem::owner_read);
    ASSERT_TRUE(stat.permissions() & boost::filesystem::owner_write);
    ASSERT_TRUE(stat.permissions() & boost::filesystem::owner_exe);
    ASSERT_TRUE(stat.permissions() & boost::filesystem::group_read);
    ASSERT_FALSE(stat.permissions() & boost::filesystem::group_write);
    ASSERT_TRUE(stat.permissions() & boost::filesystem::group_exe);
    ASSERT_TRUE(stat.permissions() & boost::filesystem::others_read);
    ASSERT_FALSE(stat.permissions() & boost::filesystem::others_write);
    ASSERT_TRUE(stat.permissions() & boost::filesystem::others_exe);
}

UTEST_P(FileSinks, TestValidWriteInFile) { LogSingleMessageAndCheck(); }

UTEST_P(FileSinks, TestReopenWithTruncate) {
    LogSingleMessageAndCheck();

    EXPECT_NO_THROW(Sink().Reopen(logging::impl::ReopenMode::kTruncate));
    EXPECT_EQ(test::ReadFromFile(Filename()), test::Messages());
}

UTEST_P(FileSinks, TestReopenWithTruncateWrite) {
    LogSingleMessageAndCheck();

    EXPECT_NO_THROW(Sink().Reopen(logging::impl::ReopenMode::kTruncate));
    EXPECT_EQ(test::ReadFromFile(Filename()), test::Messages());

    LogSingleMessageAndCheck();
}

UTEST_P(FileSinks, TestReopenWithoutTruncate) {
    LogSingleMessageAndCheck();

    EXPECT_NO_THROW(Sink().Reopen(logging::impl::ReopenMode::kAppend));
    EXPECT_EQ(test::ReadFromFile(Filename()), test::Messages("message"));
}

UTEST_P(FileSinks, TestReopenBeforeRemove) {
    LogSingleMessageAndCheck();

    fs::blocking::RemoveSingleFile(Filename());
    EXPECT_NO_THROW(Sink().Reopen(logging::impl::ReopenMode::kAppend));
    EXPECT_EQ(test::ReadFromFile(Filename()), test::Messages());
}

UTEST_P(FileSinks, TestReopenBeforeRemoveCreate) {
    LogSingleMessageAndCheck();

    fs::blocking::RemoveSingleFile(Filename());
    {
        // Re-create file manually
        [[maybe_unused]] const auto fd = fs::blocking::FileDescriptor::Open(
            Filename(), {fs::blocking::OpenFlag::kWrite, fs::blocking::OpenFlag::kCreateIfNotExists}
        );
    }

    EXPECT_NO_THROW(Sink().Reopen(logging::impl::ReopenMode::kAppend));
    EXPECT_EQ(test::ReadFromFile(Filename()), test::Messages());

    LogSingleMessageAndCheck();
}

UTEST_P(FileSinks, TestReopenBeforeRemoveAndWrite) {
    LogSingleMessageAndCheck();

    fs::blocking::RemoveSingleFile(Filename());
    EXPECT_NO_THROW(Sink().Reopen(logging::impl::ReopenMode::kAppend));
    EXPECT_EQ(test::ReadFromFile(Filename()), test::Messages());

    LogSingleMessageAndCheck();
}

UTEST_P(FileSinks, TestReopenMoveFile) {
    LogSingleMessageAndCheck();

    const std::string filename_2 = GetTempRootPath() + "/temp_file_2";
    ASSERT_TRUE(Filename() != filename_2);
    fs::blocking::Rename(Filename(), filename_2);

    EXPECT_NO_THROW(Sink().Log({"message 2\n", logging::Level::kInfo}));
    EXPECT_NO_THROW(Sink().Flush());
    EXPECT_EQ(test::ReadFromFile(filename_2), test::Messages("message", "message 2"));

    EXPECT_NO_THROW(Sink().Reopen(logging::impl::ReopenMode::kAppend));
    EXPECT_EQ(test::ReadFromFile(Filename()), test::Messages());

    EXPECT_NO_THROW(Sink().Log({"message\n", logging::Level::kWarning}));
    EXPECT_NO_THROW(Sink().Flush());
    EXPECT_EQ(test::ReadFromFile(Filename()), test::Messages("message"));
}

UTEST_P(FileSinks, TestValidWriteMultiInFile) {
    EXPECT_NO_THROW(Sink().Log({"message\n", logging::Level::kWarning}));
    EXPECT_NO_THROW(Sink().Log({"message 2\n", logging::Level::kInfo}));
    EXPECT_NO_THROW(Sink().Log({"message 3\n", logging::Level::kCritical}));
    EXPECT_NO_THROW(Sink().Flush());

    EXPECT_EQ(test::ReadFromFile(Filename()), test::Messages("message", "message 2", "message 3"));
}

INSTANTIATE_UTEST_SUITE_P(
    /* no prefix */,
    FileSinks,
    testing::Values(SinkFactory{"FileSink", MakeFileSink}, SinkFactory{"BufferedFileSink", MakeBufferedFileSink}),
    utest::PrintTestName()
);

USERVER_NAMESPACE_END
