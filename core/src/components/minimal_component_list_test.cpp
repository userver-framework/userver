#include <userver/components/minimal_component_list.hpp>

#include <fmt/format.h>

#include <userver/components/run.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>

#include <components/component_list_test.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kConfigVarsTemplate = R"(
  logger_file_path: '@null'
)";

}  // namespace

TEST_F(ComponentList, Minimal) {
  const auto temp_root = fs::blocking::TempDirectory::Create();
  const std::string config_vars_path =
      temp_root.GetPath() + "/config_vars.yaml";
  const std::string static_config =
      std::string{tests::kMinimalStaticConfig} + config_vars_path + '\n';

  fs::blocking::RewriteFileContents(config_vars_path, kConfigVarsTemplate);

  components::RunOnce(components::InMemoryConfig{static_config},
                      components::MinimalComponentList());
}

USERVER_NAMESPACE_END
