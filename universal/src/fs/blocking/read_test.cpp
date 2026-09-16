#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <variant>

#include <gtest/gtest.h>
#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/operations.hpp>

#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/utils/overloaded.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs::blocking {

class TestReadRecursiveFilesInfoWithData : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = TempDirectory::Create(
            boost::filesystem::temp_directory_path().string(),
            "userver-test-fs-blocking-read-recursive"
        );
        dir_path_ = boost::filesystem::absolute(tmp_dir_.GetPath());
        dir_path_str_ = dir_path_.string();
    }

    void TearDown() override {
        for (auto iter = boost::filesystem::recursive_directory_iterator(tmp_dir_.GetPath());
             iter != boost::filesystem::recursive_directory_iterator();
             ++iter)
        {
            boost::filesystem::permissions(iter->path(), boost::filesystem::add_perms | boost::filesystem::owner_read);
        }
        std::move(tmp_dir_).Remove();
    }

    const boost::filesystem::path& GetDirPath() const { return dir_path_; }
    const std::string& GetDirPathStr() const { return dir_path_str_; }

private:
    TempDirectory tmp_dir_;
    boost::filesystem::path dir_path_;
    std::string dir_path_str_;
};

namespace {

std::string DataToString(const std::variant<std::string, std::filesystem::path>& data) {
    return utils::Visit(
        data,
        [](const std::string& contents) { return contents; },
        [](const std::filesystem::path& path) { return path.string(); }
    );
}

std::string ToString(const FileInfoWithDataMap& file_info_with_data_map) {
    std::ostringstream ostr;
    ostr << "{";
    bool is_first_item = true;
    for (const auto& [file_path, info_ptr] : file_info_with_data_map) {
        if (!is_first_item) {
            ostr << ",";
        } else {
            is_first_item = false;
        }
        if (!info_ptr) {
            ostr << "{\"" << file_path << "\"}";
            continue;
        }
        ostr << "{\"" << file_path << "\",\"" << DataToString(info_ptr->data_or_path) << "\",\"" << info_ptr->extension
             << "\"}";
    }
    ostr << "}";
    return ostr.str();
}

void CreateTestFiles(const boost::filesystem::path& dir_path, const FileInfoWithDataMap& files_to_create) {
    for (const auto& [path_str, info] : files_to_create) {
        const auto file_path = dir_path / path_str;
        boost::filesystem::create_directories(file_path.parent_path());
        std::ofstream output_file(file_path.string());
        output_file << std::get<std::string>(info->data_or_path);
    }
}

FileInfoWithDataConstPtr CreateFileInfo(std::string data, std::string ext) {
    return std::make_shared<FileInfoWithData>(FileInfoWithData{std::move(data), std::move(ext)});
}

::testing::AssertionResult AssertionFailure(
    const char* lhs_expr,
    const char* rhs_expr,
    const FileInfoWithDataMap& lhs,
    const FileInfoWithDataMap& rhs
) {
    return ::testing::AssertionFailure()
           << "'" << lhs_expr << "' and '" << rhs_expr << "' are not equal:\n"
           << ToString(lhs) << "\n and\n"
           << ToString(rhs);
}

::testing::AssertionResult AssertionEqual(
    const char* lhs_expr,
    const char* rhs_expr,
    const FileInfoWithDataMap& lhs,
    const FileInfoWithDataMap& rhs
) {
    if (lhs.size() != rhs.size()) {
        return AssertionFailure(lhs_expr, rhs_expr, lhs, rhs);
    }
    for (const auto& [lhs_path, lhs_info] : lhs) {
        auto rhs_iter = rhs.find(lhs_path);
        if (rhs_iter == rhs.end() || static_cast<bool>(lhs_info) != static_cast<bool>(rhs_iter->second)) {
            return AssertionFailure(lhs_expr, rhs_expr, lhs, rhs);
        }
        if (!lhs_info) {
            continue;
        }
        const auto& rhs_info = rhs_iter->second;
        if (std::tie(lhs_info->data_or_path, lhs_info->extension) !=
            std::tie(rhs_info->data_or_path, rhs_info->extension))
        {
            return AssertionFailure(lhs_expr, rhs_expr, lhs, rhs);
        }
    }
    return ::testing::AssertionSuccess();
}

}  // namespace

TEST_F(TestReadRecursiveFilesInfoWithData, NormalRead) {
    const FileInfoWithDataMap existing_files{
        {"/.file_1.abc", CreateFileInfo("/.file_1.abc content", ".abc")},
        {"/file_2.def", CreateFileInfo("/file_2.def content", ".def")},
        {"/subdir1/.file_3", CreateFileInfo("/subdir1/.file_3 content", ".file_3")},
        {"/subdir2/subsubdir1/file_4.x.y.z", CreateFileInfo("/subdir2/subsubdir1/file_4.x.y.z content", ".z")}
    };
    CreateTestFiles(GetDirPath(), existing_files);

    {
        const FileInfoWithDataMap& expected = existing_files;
        auto read_result = ReadRecursiveFilesInfoWithData(GetDirPathStr(), SettingsReadFile::kNone);
        EXPECT_PRED_FORMAT2(AssertionEqual, expected, read_result);
    }

    {
        const FileInfoWithDataMap expected{
            {"/file_2.def", CreateFileInfo("/file_2.def content", ".def")},
            {"/subdir2/subsubdir1/file_4.x.y.z", CreateFileInfo("/subdir2/subsubdir1/file_4.x.y.z content", ".z")}
        };
        auto read_result = ReadRecursiveFilesInfoWithData(GetDirPathStr(), SettingsReadFile::kSkipHidden);
        EXPECT_PRED_FORMAT2(AssertionEqual, expected, read_result);
    }
}

TEST_F(TestReadRecursiveFilesInfoWithData, ReadWithException) {
    const FileInfoWithDataMap existing_files{
        {"/file_1.abc", CreateFileInfo("/file_1.abc content", ".abc")},
        {"/file_2.def", CreateFileInfo("/file_2.def content", ".def")}
    };
    CreateTestFiles(GetDirPath(), existing_files);
    boost::filesystem::permissions(
        GetDirPath() / "file_1.abc",
        boost::filesystem::remove_perms | boost::filesystem::owner_read
    );

    // fails to read the content of 'file_1.abc' due to the lack of a read permission
    EXPECT_THROW(ReadRecursiveFilesInfoWithData(GetDirPathStr(), SettingsReadFile::kSkipHidden), std::runtime_error);
}

TEST_F(TestReadRecursiveFilesInfoWithData, MaxSizeToCache) {
    constexpr std::string_view kSmallContent = "small";
    constexpr std::string_view kLargeContent = "content that does not fit";

    const auto small_path = GetDirPath() / "small.txt";
    const auto large_path = GetDirPath() / "large.txt";
    {
        std::ofstream output_file(small_path.string());
        output_file << kSmallContent;
    }
    {
        std::ofstream output_file(large_path.string());
        output_file << kLargeContent;
    }

    const auto read_result = ReadRecursiveFilesInfoWithData(
        GetDirPathStr(),
        SettingsReadFile::kSkipHidden,
        /*max_size_to_cache=*/kSmallContent.size()
    );

    ASSERT_EQ(read_result.size(), 2);

    const auto small_it = read_result.find("/small.txt");
    ASSERT_NE(small_it, read_result.end());
    ASSERT_TRUE(std::holds_alternative<std::string>(small_it->second->data_or_path));
    EXPECT_EQ(std::get<std::string>(small_it->second->data_or_path), kSmallContent);
    EXPECT_EQ(small_it->second->extension, ".txt");

    const auto large_it = read_result.find("/large.txt");
    ASSERT_NE(large_it, read_result.end());
    ASSERT_TRUE(std::holds_alternative<std::filesystem::path>(large_it->second->data_or_path));
    EXPECT_EQ(
        std::filesystem::absolute(std::filesystem::path{large_path.string()}),
        std::get<std::filesystem::path>(large_it->second->data_or_path)
    );
    EXPECT_EQ(large_it->second->extension, ".txt");
}

}  // namespace fs::blocking

USERVER_NAMESPACE_END
