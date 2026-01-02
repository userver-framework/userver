#include <fstream>
#include <sstream>

#include <gtest/gtest.h>
#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/operations.hpp>

#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_directory.hpp>

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
        ostr << "{\"" << file_path << "\",\"" << info_ptr->data << "\",\"" << info_ptr->extension << "\"}";
    }
    ostr << "}";
    return ostr.str();
}

void CreateTestFiles(const boost::filesystem::path& dir_path, const FileInfoWithDataMap& files_to_create) {
    for (const auto& [path_str, info] : files_to_create) {
        const auto file_path = dir_path / path_str;
        boost::filesystem::create_directories(file_path.parent_path());
        std::ofstream output_file(file_path.string());
        output_file << info->data;
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
        if (rhs_iter == rhs.end() || bool(lhs_info) != bool(rhs_iter->second)) {
            return AssertionFailure(lhs_expr, rhs_expr, lhs, rhs);
        }
        if (!lhs_info) {
            continue;
        }
        const auto& rhs_info = rhs_iter->second;
        if (std::tie(lhs_info->data, lhs_info->extension) != std::tie(rhs_info->data, rhs_info->extension)) {
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
        const FileInfoWithDataMap expected = existing_files;
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

}  // namespace fs::blocking

USERVER_NAMESPACE_END
