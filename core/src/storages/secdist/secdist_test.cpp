#include <gtest/gtest.h>

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

TEST(SecdistConfig, Sample) {
  auto temp_file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(temp_file.GetPath(),
                                    /** [Secdist Usage Sample - json] */ R"~(
  {
      "user-passwords": {
          "username": "password",
          "another username": "another password"
      }
  }
  )~");  /// [Secdist Usage Sample - json]

  storages::secdist::SecdistConfig secdist_config(temp_file.GetPath(), false,
                                                  std::nullopt);

  /// [Secdist Usage Sample - SecdistConfig]
  auto& user_passwords = secdist_config.Get<UserPasswords>();

  const auto password = UserPasswords::Password{"password"};
  EXPECT_TRUE(user_passwords.IsMatching("username", password));
  EXPECT_FALSE(user_passwords.IsMatching("username2", password));
  /// [Secdist Usage Sample - SecdistConfig]
}
