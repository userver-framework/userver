#include <userver/components/minimal_component_list.hpp>

#include <fmt/format.h>

#include <userver/components/run.hpp>
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

const std::string kConfigVariables = fmt::format(
    "runtime_config_path: {}\nlogger_file_path: '@null'", kRuntimeConfingPath);

const std::string kStaticConfig =
    std::string{tests::kMinimalStaticConfig} + kConfigVariablesPath + '\n';

}  // namespace

TEST(CommonComponentList, Minimal) {
  tests::LogLevelGuard guard;

  fs::blocking::RewriteFileContents(kRuntimeConfingPath, tests::kRuntimeConfig);
  fs::blocking::RewriteFileContents(kConfigVariablesPath, kConfigVariables);

  components::RunOnce(components::InMemoryConfig{kStaticConfig},
                      components::MinimalComponentList());
}

USERVER_NAMESPACE_END
