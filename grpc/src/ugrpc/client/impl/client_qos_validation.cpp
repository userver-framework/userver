#include <ugrpc/client/impl/client_qos_validation.hpp>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/default_dict.hpp>
#include <userver/utils/regex.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

std::string_view ToString(RpcPathValidationError error) {
    switch (error) {
        case RpcPathValidationError::kEmptyPath:
            return "Path is empty";
        case RpcPathValidationError::kLeadingSlash:
            return "Path should not start with a '/'";
        case RpcPathValidationError::kMissingSlash:
            return "Path must contain exactly one '/' separator, it had been lost";
        case RpcPathValidationError::kTooManySlashes:
            return "Path must contain exactly one '/' separator";
        case RpcPathValidationError::kMissingDotInService:
            return "Service name part must contain at least one '.'";
        case RpcPathValidationError::kEmptyMethod:
            return "Method name part must not be empty";
        case RpcPathValidationError::kInvalidFormat:
            return "Path does not match the expected format";
        case RpcPathValidationError::kNotFound:
            return "Method does not exist in service";
    }
    return "Unknown error";
}

std::optional<RpcPathValidationError> IsValidRpcMethodPath(std::string_view path) {
    if (path == utils::kDefaultDictDefaultName) {
        return std::nullopt;
    }

    if (path.empty()) {
        return RpcPathValidationError::kEmptyPath;
    }

    if (path.front() == '/') {
        return RpcPathValidationError::kLeadingSlash;
    }

    const auto slash_pos = path.find('/');
    if (slash_pos == std::string_view::npos) {
        return RpcPathValidationError::kMissingSlash;
    }

    if (slash_pos == path.size() - 1) {
        return RpcPathValidationError::kEmptyMethod;
    }

    if (path.find('/', slash_pos + 1) != std::string_view::npos) {
        return RpcPathValidationError::kTooManySlashes;
    }

    const auto service_part = path.substr(0, slash_pos);
    if (service_part.find('.') == std::string_view::npos) {
        return RpcPathValidationError::kMissingDotInService;
    }

    const auto method_part = path.substr(slash_pos + 1);
    if (method_part.empty()) {
        return RpcPathValidationError::kEmptyMethod;
    }

    static const utils::regex
        kRpcPathRegex(R"(^[a-zA-Z_][a-zA-Z0-9_]*(\.[a-zA-Z_][a-zA-Z0-9_]*)+/[a-zA-Z_][a-zA-Z0-9_]*$)");
    if (!utils::regex_match(path, kRpcPathRegex)) {
        return RpcPathValidationError::kInvalidFormat;
    }

    return std::nullopt;
}

bool IsMethodOfService(std::string_view method_path, std::string_view service_name) {
    if (method_path == utils::kDefaultDictDefaultName) {
        return false;  // __default__ doesn't belong to any specific service
    }

    const auto slash_pos = method_path.find('/');
    UASSERT(slash_pos != std::string_view::npos);

    const auto method_service_part = method_path.substr(0, slash_pos);
    return method_service_part == service_name;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
