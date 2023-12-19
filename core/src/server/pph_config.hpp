#pragma once

#include <string>
#include <unordered_map>

#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {

class PassphraseConfig final {
 public:
  using Passphrase = utils::NonLoggable<class PassphraseT, std::string>;

  explicit PassphraseConfig(const formats::json::Value& doc)
      : passphrases_(
            doc["passphrases"].As<std::unordered_map<std::string, Passphrase>>(
                {})) {}

  Passphrase GetPassphrase(const std::string& name) const {
    try {
      return passphrases_.at(name);
    } catch (const std::out_of_range& e) {
      LOG_ERROR() << "No passphrase for name '" << name << "'";
      throw;
    }
  }

 private:
  std::unordered_map<std::string, Passphrase> passphrases_;
};

}  // namespace server

USERVER_NAMESPACE_END
