#include <userver/chaotic/openapi/client/digest_auth.hpp>

#include <fmt/format.h>

#include <userver/clients/http/request.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::client {

namespace {

DigestAuthCredentials ParseCredentials(const formats::json::Value& doc) {
    return DigestAuthCredentials{
        doc["username"].As<std::string>(),
        DigestAuthCredentials::Password{doc["password"].As<std::string>()},
    };
}

}  // namespace

DigestAuthSecdistConfig::DigestAuthSecdistConfig(const formats::json::Value& doc) {
    const auto& root = doc["http_digest"];
    if (root.IsMissing() || root.IsNull()) {
        return;
    }
    for (const auto& [client_name, schemes_node] : Items(root)) {
        DigestAuthCredentialsByScheme by_scheme;
        for (const auto& [scheme_name, creds_node] : Items(schemes_node)) {
            by_scheme.emplace(scheme_name, ParseCredentials(creds_node));
        }
        credentials_.emplace(client_name, std::move(by_scheme));
    }
}

const DigestAuthCredentials& DigestAuthSecdistConfig::GetCredentialsOrThrow(
    const std::string& client_name,
    const std::string& scheme_name
) const {
    const auto* by_scheme = utils::FindOrNullptr(credentials_, client_name);
    if (by_scheme == nullptr) {
        throw std::runtime_error{fmt::format(
            "chaotic-openapi digest auth: no secdist entry for client '{0}' "
            "(key: http_digest.{0})",
            client_name
        )};
    }
    const auto* credentials = utils::FindOrNullptr(*by_scheme, scheme_name);
    if (credentials == nullptr) {
        throw std::runtime_error{fmt::format(
            "chaotic-openapi digest auth: no secdist entry for client '{0}', "
            "scheme '{1}' "
            "(key: http_digest.{0}.{1})",
            client_name,
            scheme_name
        )};
    }
    return *credentials;
}

DigestAuthProvider::DigestAuthProvider(const DigestAuthSecdistConfig& credentials) : credentials_(credentials) {}

void DigestAuthProvider::Prepare(std::string_view client_name, std::string_view scheme_name) {
    credentials_.GetCredentialsOrThrow(std::string{client_name}, std::string{scheme_name});
}

void DigestAuthProvider::Apply(clients::http::Request& request, const SecurityRequirement& requirement) {
    const auto& credentials =
        credentials_.GetCredentialsOrThrow(std::string{requirement.client_name}, std::string{requirement.scheme_name});
    request.http_auth_type(
        clients::http::HttpAuthType::kDigest,
        /*auth_only=*/false,
        credentials.username,
        credentials.password.GetUnderlying()
    );
}

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
