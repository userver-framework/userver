#include <userver/utest/utest.hpp>

#include <atomic>
#include <cstdlib>

#include <userver/engine/sleep.hpp>
#include <userver/engine/subprocess/environment_variables.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>

#include <userver/utest/using_namespace_userver.hpp>

/// [UserPasswords]
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/storages/secdist/secdist.hpp>

#include <userver/crypto/algorithm.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/strong_typedef.hpp>

class UserPasswords {
 public:
  using Password = utils::NonLoggable<class PasswordTag, std::string>;

  UserPasswords(const formats::json::Value& doc)
      : user_password_(doc["user-passwords"].As<Storage>()) {}

  bool IsMatching(const std::string& user, const Password& password) const {
    const auto* ptr = utils::FindOrNullptr(user_password_, user);
    return ptr && crypto::algorithm::AreStringsEqualConstTime(
                      ptr->GetUnderlying(), password.GetUnderlying());
  }

 private:
  using Storage = std::unordered_map<std::string, Password>;
  Storage user_password_;
};
/// [UserPasswords]

USERVER_NAMESPACE_BEGIN

namespace {

const std::string kSecdistJson =
    /** [Secdist Usage Sample - json] */ R"~(
  {
      "user-passwords": {
          "username": "password",
          "another username": "another password"
      }
  }
  )~";  /// [Secdist Usage Sample - json]

const std::string kSecdistYaml =
    /** [Secdist Usage Sample - yaml] */ R"~(
  user-passwords:
    username: drowssap
    another username: drowssap rehtona
  )~";  /// [Secdist Usage Sample - yaml]

}  // namespace

TEST(SecdistConfig, Sample) {
  auto temp_file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(temp_file.GetPath(), kSecdistJson);

  storages::secdist::DefaultLoader provider{
      {temp_file.GetPath(), storages::secdist::SecdistFormat::kJson, false,
       std::nullopt}};
  storages::secdist::SecdistConfig secdist_config{{&provider}};
  /// [Secdist Usage Sample - SecdistConfig]
  const auto& user_passwords = secdist_config.Get<UserPasswords>();

  const auto password = UserPasswords::Password{"password"};
  EXPECT_TRUE(user_passwords.IsMatching("username", password));
  EXPECT_FALSE(user_passwords.IsMatching("username2", password));
  /// [Secdist Usage Sample - SecdistConfig]
}

TEST(SecdistYamlConfig, Sample) {
  auto temp_file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(temp_file.GetPath(), kSecdistYaml);

  storages::secdist::DefaultLoader provider{
      {temp_file.GetPath(), storages::secdist::SecdistFormat::kYaml, false,
       std::nullopt}};
  storages::secdist::SecdistConfig secdist_config{{&provider}};

  /// [Secdist Usage Sample - SecdistConfig]
  const auto& user_passwords = secdist_config.Get<UserPasswords>();

  const auto password = UserPasswords::Password{"drowssap"};
  const auto another_password = UserPasswords::Password{"drowssap rehtona"};
  EXPECT_TRUE(user_passwords.IsMatching("username", password));
  EXPECT_FALSE(user_passwords.IsMatching("username2", password));
  EXPECT_TRUE(user_passwords.IsMatching("another username", another_password));
  /// [Secdist Usage Sample - SecdistConfig]
}

UTEST(SecdistConfig, EnvironmentVariable) {
  static const std::string kVarName = "SECRET";

  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  ASSERT_EQ(setenv(kVarName.c_str(), kSecdistJson.c_str(), 1), 0);
  engine::subprocess::UpdateCurrentEnvironmentVariables();

  storages::secdist::DefaultLoader provider{
      {"", storages::secdist::SecdistFormat::kJson, false, kVarName}};
  storages::secdist::SecdistConfig secdist_config{{&provider}};

  const auto& user_passwords = secdist_config.Get<UserPasswords>();

  const auto password = UserPasswords::Password{"password"};
  EXPECT_TRUE(user_passwords.IsMatching("username", password));
  EXPECT_FALSE(user_passwords.IsMatching("username2", password));
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  ASSERT_EQ(unsetenv(kVarName.c_str()), 0);

  engine::subprocess::UpdateCurrentEnvironmentVariables();
}

UTEST(SecdistConfig, FileAndEnvironmentVariable) {
  const std::string kSecdistFileJson = R"~(
  {
      "user-passwords": {
          "username": "password_old",
          "another username": "another password"
      }
  }
  )~";

  const std::string kSecdistEnvVarJson = R"~(
  {
      "user-passwords": {
          "username": "password_updated",
          "username_new": "xyz"
      }
  }
  )~";

  auto temp_file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(temp_file.GetPath(), kSecdistFileJson);

  static const std::string kVarName = "SECRET";

  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  ASSERT_EQ(setenv(kVarName.c_str(), kSecdistEnvVarJson.c_str(), 1), 0);
  engine::subprocess::UpdateCurrentEnvironmentVariables();

  storages::secdist::DefaultLoader provider{
      {temp_file.GetPath(), storages::secdist::SecdistFormat::kJson, false,
       kVarName}};
  storages::secdist::SecdistConfig secdist_config{{&provider}};

  const auto& user_passwords = secdist_config.Get<UserPasswords>();

  const auto password = UserPasswords::Password{"password_updated"};
  const auto another_password = UserPasswords::Password{"another password"};
  const auto password_xyz = UserPasswords::Password{"xyz"};
  EXPECT_TRUE(user_passwords.IsMatching("username", password));
  EXPECT_FALSE(user_passwords.IsMatching("username2", password));
  EXPECT_TRUE(user_passwords.IsMatching("username_new", password_xyz));
  EXPECT_TRUE(user_passwords.IsMatching("another username", another_password));

  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  ASSERT_EQ(unsetenv(kVarName.c_str()), 0);
  engine::subprocess::UpdateCurrentEnvironmentVariables();
}

UTEST(Secdist, WithoutUpdates) {
  auto temp_file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(temp_file.GetPath(), kSecdistJson);

  storages::secdist::DefaultLoader provider{
      {temp_file.GetPath(), storages::secdist::SecdistFormat::kJson, false,
       std::nullopt}};
  storages::secdist::Secdist secdist{{&provider}};

  const auto& secdist_config = secdist.Get();

  const auto& user_passwords = secdist_config.Get<UserPasswords>();

  const auto password = UserPasswords::Password{"password"};
  EXPECT_TRUE(user_passwords.IsMatching("username", password));
  EXPECT_FALSE(user_passwords.IsMatching("username2", password));
}

UTEST(Secdist, DynamicUpdate) {
  const std::string kSecdistInitJson = R"~(
  {
      "user-passwords": {
          "username": "password_old",
          "another username": "another password"
      }
  }
  )~";

  const std::string kSecdistUpdateJson = R"~(
  {
      "user-passwords": {
          "username": "password_updated",
          "username_new": "xyz"
      }
  }
  )~";

  struct SecdistConfigStorage {
    void OnSecdistUpdate(
        const storages::secdist::SecdistConfig& secdist_config_update) {
      if (updates_counter == 1) {
        // prevents test flaps
        while (!file_updated.load()) {
          engine::SleepFor(std::chrono::milliseconds(1));
        }
      }

      if (updates_counter < 2) secdist_config = secdist_config_update;
      updates_counter++;
    };

    storages::secdist::SecdistConfig secdist_config;
    std::atomic<int> updates_counter{0};
    std::atomic<bool> file_updated{false};
  };

  SecdistConfigStorage storage;

  auto temp_file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(temp_file.GetPath(), kSecdistInitJson);

  storages::secdist::DefaultLoader provider{
      {temp_file.GetPath(), storages::secdist::SecdistFormat::kJson, false,
       std::nullopt, &engine::current_task::GetTaskProcessor()}};
  storages::secdist::Secdist secdist{
      {&provider, std::chrono::milliseconds(100)}};

  auto subscriber = secdist.UpdateAndListen(
      &storage, "test/update_secdist", &SecdistConfigStorage::OnSecdistUpdate);
  EXPECT_EQ(storage.updates_counter.load(), 1);
  const auto& secdist_config = secdist.Get();

  const auto& user_passwords = secdist_config.Get<UserPasswords>();

  const auto password_old = UserPasswords::Password{"password_old"};
  const auto password_updated = UserPasswords::Password{"password_updated"};
  const auto another_password = UserPasswords::Password{"another password"};
  const auto password_xyz = UserPasswords::Password{"xyz"};

  auto check_user_passwords = [&](const UserPasswords& user_passwords) {
    EXPECT_TRUE(user_passwords.IsMatching("username", password_old));
    EXPECT_FALSE(user_passwords.IsMatching("username2", password_old));
    EXPECT_FALSE(user_passwords.IsMatching("username_new", password_xyz));
    EXPECT_TRUE(
        user_passwords.IsMatching("another username", another_password));
  };

  check_user_passwords(user_passwords);

  {
    auto snapshot = secdist.GetSnapshot();
    const auto& dynamic_secdist_config = *snapshot;

    check_user_passwords(dynamic_secdist_config.Get<UserPasswords>());
  }

  fs::blocking::RewriteFileContents(temp_file.GetPath(), kSecdistUpdateJson);
  ASSERT_EQ(storage.updates_counter.load(), 1);
  storage.file_updated = true;
  while (storage.updates_counter.load() < 2) {
    engine::SleepFor(std::chrono::milliseconds(1));
  }
  const auto& current_secdist = storage.secdist_config;
  const auto& updated_user_passwords = current_secdist.Get<UserPasswords>();

  EXPECT_TRUE(updated_user_passwords.IsMatching("username", password_updated));
  EXPECT_TRUE(updated_user_passwords.IsMatching("username_new", password_xyz));
  EXPECT_FALSE(
      updated_user_passwords.IsMatching("another username", another_password));

  subscriber.Unsubscribe();
}

USERVER_NAMESPACE_END
