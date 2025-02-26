#include <userver/utils/statistics/metadata.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/string_literal.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {
namespace {

constexpr utils::StringLiteral kMetadata = "$meta";

constexpr utils::StringLiteral kMetadataSolomonSkip = "solomon_skip";
constexpr utils::StringLiteral kMetadataSolomonRename = "solomon_rename";
constexpr utils::StringLiteral kMetadataSolomonLabel = "solomon_label";
constexpr utils::StringLiteral kMetadataSolomonChildrenLabels = "solomon_children_labels";

}  // namespace

void SolomonSkip(formats::json::ValueBuilder& stats_node) {
    stats_node[std::string{kMetadata}][std::string{kMetadataSolomonSkip}] = true;
}

void SolomonSkip(formats::json::ValueBuilder&& stats_node) { SolomonSkip(stats_node); }

void SolomonRename(formats::json::ValueBuilder& stats_node, const std::string& new_name) {
    stats_node[std::string{kMetadata}][std::string{kMetadataSolomonRename}] = new_name;
}

void SolomonRename(formats::json::ValueBuilder&& stats_node, const std::string& new_name) {
    SolomonRename(stats_node, new_name);
}

void SolomonLabelValue(formats::json::ValueBuilder& stats_node, const std::string& label_name) {
    UASSERT_MSG(label_name.size() <= 32, "Max. length of Solomon label node is 32 chars");
    stats_node[std::string{kMetadata}][std::string{kMetadataSolomonLabel}] = label_name;
}

void SolomonLabelValue(formats::json::ValueBuilder&& stats_node, const std::string& label_name) {
    SolomonLabelValue(stats_node, label_name);
}

void SolomonChildrenAreLabelValues(formats::json::ValueBuilder& stats_node, const std::string& label_name) {
    UASSERT_MSG(label_name.size() <= 32, "Max. length of Solomon label node is 32 chars");
    stats_node[std::string{kMetadata}][std::string{kMetadataSolomonChildrenLabels}] = label_name;
}

void SolomonChildrenAreLabelValues(formats::json::ValueBuilder&& stats_node, const std::string& label_name) {
    SolomonChildrenAreLabelValues(stats_node, label_name);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
