#include <userver/components/minimal_component_list.hpp>

#include <fmt/format.h>

#include <userver/components/run.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>

#include <components/component_list_test.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const auto kTmpDir = fs::blocking::TempDirectory::Create();
const std::string kRuntimeConfingPath =
    kTmpDir.GetPath() + "/runtime_config.json";
const std::string kConfigVariablesPath =
    kTmpDir.GetPath() + "/config_vars.json";
const std::string kLogsPath = kTmpDir.GetPath() + "/log.txt";

const std::string kConfigVariables =
    fmt::format("runtime_config_path: {}\nlogger_file_path: {}",
                kRuntimeConfingPath, kLogsPath);

const std::string kStaticConfig =
    std::string{tests::kMinimalStaticConfig} + kConfigVariablesPath + '\n';

}  // namespace

TEST(CommonComponentList, MinimalLtsvLogs) {
  {
    tests::LogLevelGuard guard{};

    fs::blocking::RewriteFileContents(kRuntimeConfingPath,
                                      tests::kRuntimeConfig);
    fs::blocking::RewriteFileContents(kConfigVariablesPath, kConfigVariables);

    components::RunOnce(components::InMemoryConfig{kStaticConfig},
                        components::MinimalComponentList());

    logging::LogFlush();
    auto logger = logging::DefaultLogger();
    UASSERT(logger.use_count() == 2);
  }

  const auto logs = fs::blocking::ReadFileContents(kLogsPath);
  EXPECT_EQ(logs.find("tskv\t"), std::string::npos) << logs;
  EXPECT_NE(logs.find("\ttext:"), std::string::npos) << logs;
}

USERVER_NAMESPACE_END
