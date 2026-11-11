#pragma once

/// @file userver/formats/json/masking.hpp
/// @brief Streaming JSON masking for bounded log output.

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

/// @brief A sequence of object member names. Arrays do not consume a segment.
using MaskingPath = std::vector<std::string>;

/// @brief Whether paths select hidden or visible subtrees.
enum class MaskingListMode {
    kBlacklist,  ///< Mask the selected paths and their complete subtrees.
    kWhitelist,  ///< Preserve only the selected paths and their subtrees.
};

struct MaskedJson;

/// @brief Immutable compiled masking rules, cheaply copyable and safe to share.
/// A whole segment "*" matches any member name unless an exact child exists.
/// Exact and wildcard branches are not merged or retried on a later mismatch.
/// An empty path selects the root. Empty blacklists preserve everything;
/// empty whitelists mask everything. Incomplete whitelist paths hide scalars.
class MaskingRules final {
public:
    /// @brief Compile paths once for reuse by subsequent masking calls.
    explicit MaskingRules(
        const std::vector<MaskingPath>& paths = {},
        MaskingListMode mode = MaskingListMode::kBlacklist
    );

    /// @brief Use the same UTF-8 replacement string for every masked value when provided.
    /// @throws std::invalid_argument If the replacement is invalid UTF-8 or too large for the JSON writer.
    MaskingRules(const std::vector<MaskingPath>& paths, MaskingListMode mode, std::optional<std::string> replacement);

private:
    struct Impl;
    std::shared_ptr<const Impl> impl_;

    friend MaskedJson MaskJson(std::string_view, const MaskingRules&, std::size_t);
};

/// @brief Masked JSON or its UTF-8-safe prefix when the output budget is reached.
struct MaskedJson final {
    std::string text;    ///< Complete JSON or a UTF-8-safe prefix without a suffix.
    bool limit_reached;  ///< Processing stopped because the output budget was reached.
};

/// @brief Mask JSON using RapidJSON streaming parsing and serialization.
/// By default, scalars become "...", objects "{...}", and arrays "[...]".
/// Output is compact JSON until @p log_limit bytes are produced; then parsing
/// stops and the result may be incomplete JSON. No suffix is added.
/// Reaching the budget, even exactly, sets @ref formats::json::MaskedJson::limit_reached;
/// the unread input suffix is not validated. A zero budget skips parsing.
/// A single input token may be fully decoded before its SAX callback runs.
/// @throws formats::json::ParseException on invalid JSON, invalid UTF-8, or
/// excessive nesting encountered before reaching the budget. Error messages
/// contain no input fragments. Rules and input are never modified.
MaskedJson MaskJson(std::string_view input, const MaskingRules& rules, std::size_t log_limit);

}  // namespace formats::json

USERVER_NAMESPACE_END
