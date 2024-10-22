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
        : passphrases_(doc["passphrases"].As<std::unordered_map<std::string, Passphrase>>({})) {}

    Passphrase GetPassphrase(const std::string& name) const {
        auto it = passphrases_.find(name);
        if (it == passphrases_.cend()) {
            auto message = fmt::format("No passphrase with name '{}' in secdist 'passphrases' entry", name);
            LOG_ERROR() << message;
            throw std::runtime_error(std::move(message));
        }

        return it->second;
    }

private:
    std::unordered_map<std::string, Passphrase> passphrases_;
};

}  // namespace server

USERVER_NAMESPACE_END
