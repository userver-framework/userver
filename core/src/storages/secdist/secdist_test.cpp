#include <utest/utest.hpp>

#include <cstdlib>

#include <userver/engine/subprocess/environment_variables.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>

/// [Secdist Usage Sample - UserPasswords]
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
    auto ptr = utils::FindOrNullptr(user_password_, user);
    return ptr && crypto::algorithm::AreStringsEqualConstTime(
                      ptr->GetUnderlying(), password.GetUnderlying());
  }

 private:
  using Storage = std::unordered_map<std::string, Password>;
  Storage user_password_;
};
/// [Secdist Usage Sample - UserPasswords]

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

}  // namespace

TEST(SecdistConfig, Sample) {
  auto temp_file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(temp_file.GetPath(), kSecdistJson);

  storages::secdist::SecdistConfig secdist_config(temp_file.GetPath(), false,
                                                  std::nullopt);

  /// [Secdist Usage Sample - SecdistConfig]
  auto& user_passwords = secdist_config.Get<UserPasswords>();

  const auto password = UserPasswords::Password{"password"};
  EXPECT_TRUE(user_passwords.IsMatching("username", password));
  EXPECT_FALSE(user_passwords.IsMatching("username2", password));
  /// [Secdist Usage Sample - SecdistConfig]
}

UTEST(SecdistConfig, EnvironmentVariable) {
  static const std::string kVarName = "SECRET";

  ASSERT_EQ(setenv(kVarName.c_str(), kSecdistJson.c_str(), 1), 0);
  engine::subprocess::UpdateCurrentEnvironmentVariables();

  storages::secdist::SecdistConfig secdist_config("", false, kVarName);

  auto& user_passwords = secdist_config.Get<UserPasswords>();

  const auto password = UserPasswords::Password{"password"};
  EXPECT_TRUE(user_passwords.IsMatching("username", password));
  EXPECT_FALSE(user_passwords.IsMatching("username2", password));

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

  ASSERT_EQ(setenv(kVarName.c_str(), kSecdistEnvVarJson.c_str(), 1), 0);
  engine::subprocess::UpdateCurrentEnvironmentVariables();

  storages::secdist::SecdistConfig secdist_config(temp_file.GetPath(), false,
                                                  kVarName);

  auto& user_passwords = secdist_config.Get<UserPasswords>();

  const auto password = UserPasswords::Password{"password_updated"};
  const auto another_password = UserPasswords::Password{"another password"};
  const auto password_xyz = UserPasswords::Password{"xyz"};
  EXPECT_TRUE(user_passwords.IsMatching("username", password));
  EXPECT_FALSE(user_passwords.IsMatching("username2", password));
  EXPECT_TRUE(user_passwords.IsMatching("username_new", password_xyz));
  EXPECT_TRUE(user_passwords.IsMatching("another username", another_password));

  ASSERT_EQ(unsetenv(kVarName.c_str()), 0);
  engine::subprocess::UpdateCurrentEnvironmentVariables();
}
