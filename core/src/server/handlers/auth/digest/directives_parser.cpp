#include <userver/server/handlers/auth/digest/directives_parser.hpp>

#include <algorithm>
#include <cctype>

#include <fmt/format.h>

#include <userver/logging/log.hpp>
#include <userver/server/handlers/auth/digest/directives.hpp>
#include <userver/server/handlers/auth/digest/exception.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest {

namespace {

constexpr utils::StringLiteral kDigestWord = "Digest";

enum class ClientDirectiveTypes {
    kUsername,
    kRealm,
    kNonce,
    kUri,
    kResponse,
    kAlgorithm,
    kCnonce,
    kOpaque,
    kQop,
    kNonceCount,
    kAuthParam,
    kUnknown
};

const utils::TrivialBiMap kClientDirectivesMap = [](auto selector) {
    return selector()
        .Case(directives::kUsername, ClientDirectiveTypes::kUsername)
        .Case(directives::kRealm, ClientDirectiveTypes::kRealm)
        .Case(directives::kNonce, ClientDirectiveTypes::kNonce)
        .Case(directives::kUri, ClientDirectiveTypes::kUri)
        .Case(directives::kResponse, ClientDirectiveTypes::kResponse)
        .Case(directives::kAlgorithm, ClientDirectiveTypes::kAlgorithm)
        .Case(directives::kCnonce, ClientDirectiveTypes::kCnonce)
        .Case(directives::kOpaque, ClientDirectiveTypes::kOpaque)
        .Case(directives::kQop, ClientDirectiveTypes::kQop)
        .Case(directives::kNonceCount, ClientDirectiveTypes::kNonceCount)
        .Case(directives::kAuthParam, ClientDirectiveTypes::kAuthParam);
};

const std::array<ClientDirectiveTypes, kClientMandatoryDirectivesNumber> kMandatoryDirectives = {
    ClientDirectiveTypes::kRealm,
    ClientDirectiveTypes::kNonce,
    ClientDirectiveTypes::kResponse,
    ClientDirectiveTypes::kUri,
    ClientDirectiveTypes::kUsername};

enum class State {
    kStateSpace,
    kStateToken,
    kStateEquals,
    kStateValue,  // if Final then OK
    kStateValueQuoted,
    kStateValueEscape,
    kStateComma,  // if Final then OK
};

}  // namespace

ContextFromClient Parser::ParseAuthInfo(std::string_view auth_header_value) {
    ContextFromClient client_context;
    State state = State::kStateSpace;
    std::string token;
    std::string value;

    UASSERT_MSG(
        auth_header_value.substr(0, kDigestWord.size()) == kDigestWord,
        fmt::format("Result is: '{}'", auth_header_value.substr(0, kDigestWord.size()))
    );
    auto directives_str = auth_header_value.substr(kDigestWord.size() + 1);
    for (const char delimiter : directives_str) {
        switch (state) {
            case State::kStateSpace:
                if (std::isalnum(delimiter) || delimiter == '_' || delimiter == '-') {
                    token += delimiter;
                    state = State::kStateToken;
                } else if (std::isspace(delimiter)) {
                    // Skip
                } else
                    throw ParseException("Invalid header format");
                break;

            case State::kStateToken:
                if (delimiter == '=') {
                    state = State::kStateEquals;
                } else if (std::isalnum(delimiter) || delimiter == '_' || delimiter == '-') {
                    token += delimiter;
                } else
                    throw ParseException("Invalid header format");
                break;

            case State::kStateEquals:
                if (std::isalnum(delimiter) || delimiter == '_') {
                    value += delimiter;
                    state = State::kStateValue;
                } else if (delimiter == '"') {
                    state = State::kStateValueQuoted;
                } else
                    throw ParseException("Invalid header format");
                break;

            case State::kStateValueQuoted:
                if (delimiter == '\\') {
                    state = State::kStateValueEscape;
                } else if (delimiter == '"') {
                    PushToClientContext(std::move(token), std::move(value), client_context);
                    token.clear();
                    value.clear();
                    state = State::kStateComma;
                } else {
                    value += delimiter;
                }
                break;

            case State::kStateValueEscape:
                value += delimiter;
                state = State::kStateValueQuoted;
                break;

            case State::kStateValue:
                if (std::isspace(delimiter)) {
                    PushToClientContext(std::move(token), std::move(value), client_context);
                    token.clear();
                    value.clear();
                    state = State::kStateComma;
                } else if (delimiter == ',') {
                    PushToClientContext(std::move(token), std::move(value), client_context);
                    token.clear();
                    value.clear();
                    state = State::kStateSpace;
                } else {
                    value += delimiter;
                }
                break;

            case State::kStateComma:
                if (delimiter == ',') {
                    state = State::kStateSpace;
                } else if (std::isspace(delimiter)) {
                    // Skip
                } else
                    throw ParseException("Invalid header format");
                break;
        }
    }

    if (state == State::kStateValue) {
        PushToClientContext(std::move(token), std::move(value), client_context);
    }

    if (state != State::kStateValue && state != State::kStateComma) {
        throw ParseException("Invalid header format");
    }

    CheckMandatoryDirectivesPresent();
    CheckDuplicateDirectivesExist();

    return client_context;
}

void Parser::PushToClientContext(std::string&& directive, std::string&& value, ContextFromClient& client_context) {
    const auto directive_type =
        kClientDirectivesMap.TryFind(std::move(directive)).value_or(ClientDirectiveTypes::kUnknown);
    const auto index = static_cast<std::size_t>(directive_type);
    if (directive_type != ClientDirectiveTypes::kUnknown) directives_counter_[index]++;
    switch (directive_type) {
        case ClientDirectiveTypes::kUsername:
            client_context.username = std::move(value);
            break;
        case ClientDirectiveTypes::kRealm:
            client_context.realm = std::move(value);
            break;
        case ClientDirectiveTypes::kNonce:
            client_context.nonce = std::move(value);
            break;
        case ClientDirectiveTypes::kUri:
            client_context.uri = std::move(value);
            break;
        case ClientDirectiveTypes::kResponse:
            client_context.response = std::move(value);
            break;
        case ClientDirectiveTypes::kAlgorithm:
            client_context.algorithm = std::move(value);
            break;
        case ClientDirectiveTypes::kCnonce:
            client_context.cnonce = std::move(value);
            break;
        case ClientDirectiveTypes::kOpaque:
            client_context.opaque = std::move(value);
            break;
        case ClientDirectiveTypes::kQop:
            client_context.qop = std::move(value);
            break;
        case ClientDirectiveTypes::kNonceCount:
            client_context.nc = std::move(value);
            break;
        case ClientDirectiveTypes::kAuthParam:
            client_context.authparam = std::move(value);
            break;
        case ClientDirectiveTypes::kUnknown:
            throw ParseException("Unknown directive found");
            break;
    }
}

void Parser::CheckMandatoryDirectivesPresent() const {
    std::vector<std::string> missing_directives;
    for (auto directive_type : kMandatoryDirectives) {
        const auto index = static_cast<std::size_t>(directive_type);
        if (directives_counter_[index] == 0) {
            auto directive =
                kClientDirectivesMap.TryFind(directive_type).value_or(utils::StringLiteral{"unknown_directive"});
            UASSERT(directive != "unknown_directive");
            missing_directives.emplace_back(directive);
        }
    }
    if (!missing_directives.empty()) {
        throw MissingDirectivesException(std::move(missing_directives));
    }
}

void Parser::CheckDuplicateDirectivesExist() const {
    // NOLINTNEXTLINE(readability-qualified-auto)
    auto it = std::find_if(directives_counter_.begin(), directives_counter_.end(), [](std::size_t directive_count) {
        return directive_count > 1;
    });

    if (it != directives_counter_.end()) {
        const auto index = std::distance(directives_counter_.begin(), it);
        const auto directive_type = static_cast<ClientDirectiveTypes>(index);
        auto directive =
            kClientDirectivesMap.TryFind(directive_type).value_or(utils::StringLiteral{"unknown_directive"});
        UASSERT(directive != "unknown_directive");
        throw DuplicateDirectiveException(fmt::format("Duplicate '{}' directive found", directive));
    }
}

}  // namespace server::handlers::auth::digest

USERVER_NAMESPACE_END
