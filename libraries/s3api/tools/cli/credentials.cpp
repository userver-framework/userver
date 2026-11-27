#include "credentials.hpp"

#include <stdexcept>
#include <string>
#include <utility>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/fs/blocking/read.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::cli {

namespace {
constexpr auto kInvalidCredentials =
    "Credentials must be JSON with nonempty string fields access_key_id and "
    "secret_key";
}

Credentials ParseCredentials(const std::string& contents) {
    try {
        const auto json = formats::json::FromString(contents);
        auto access_key = json["access_key_id"].As<std::string>();
        auto secret_key = json["secret_key"].As<std::string>();
        if (access_key.empty() || secret_key.empty()) {
            throw std::invalid_argument(kInvalidCredentials);
        }
        return {std::move(access_key), Secret{std::move(secret_key)}};
    } catch (const std::exception&) {
        throw std::invalid_argument(kInvalidCredentials);
    }
}

Credentials ReadCredentials(const std::string& path) {
    std::string contents;
    try {
        contents = fs::blocking::ReadFileContents(path);
    } catch (const std::exception& error) {
        throw std::invalid_argument(
            std::string{"Cannot read --credentials-file; check the path and permissions: "} + error.what()
        );
    }
    return ParseCredentials(contents);
}

}  // namespace s3api::cli

USERVER_NAMESPACE_END
