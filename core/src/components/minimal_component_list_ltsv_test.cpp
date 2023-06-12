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

constexpr std::string_view kConfigVarsTemplate = R"(
  runtime_config_path: {0}
  logger_file_path: {1}
)";

}  // namespace

TEST_F(ComponentList, MinimalLtsvLogs) {
  const auto temp_root = fs::blocking::TempDirectory::Create();
  const std::string runtime_config_path =
      temp_root.GetPath() + "/runtime_config.json";
  const std::string config_vars_path =
      temp_root.GetPath() + "/config_vars.json";
  const std::string logs_path = temp_root.GetPath() + "/log.txt";
  const std::string static_config =
      std::string{tests::kMinimalStaticConfig} + config_vars_path + '\n';

  fs::blocking::RewriteFileContents(runtime_config_path, tests::kRuntimeConfig);
  fs::blocking::RewriteFileContents(
      config_vars_path,
      fmt::format(kConfigVarsTemplate, runtime_config_path, logs_path));

  components::RunOnce(components::InMemoryConfig{static_config},
                      components::MinimalComponentList());

  logging::LogFlush();

  const auto logs = fs::blocking::ReadFileContents(logs_path);
  EXPECT_EQ(logs.find("tskv\t"), std::string::npos) << logs;
  EXPECT_NE(logs.find("\ttext:"), std::string::npos) << logs;
}

USERVER_NAMESPACE_END
