#include <utils/statistics/metadata.hpp>

#include <utils/assert.hpp>

namespace utils {
namespace statistics {
namespace {

const std::string kMetadata = "$meta";

const std::string kMetadataSolomonSkip = "solomon_skip";
const std::string kMetadataSolomonRename = "solomon_rename";
const std::string kMetadataSolomonLabel = "solomon_label";
const std::string kMetadataSolomonChildrenLabels = "solomon_children_labels";

}  // namespace

void SolomonSkip(formats::json::ValueBuilder& stats_node) {
  stats_node[kMetadata][kMetadataSolomonSkip] = true;
}

void SolomonSkip(formats::json::ValueBuilder&& stats_node) {
  SolomonSkip(stats_node);
}

void SolomonRename(formats::json::ValueBuilder& stats_node,
                   const std::string& new_name) {
  stats_node[kMetadata][kMetadataSolomonRename] = new_name;
}

void SolomonRename(formats::json::ValueBuilder&& stats_node,
                   const std::string& new_name) {
  SolomonRename(stats_node, new_name);
}

void SolomonLabelValue(formats::json::ValueBuilder& stats_node,
                       const std::string& label_name) {
  UASSERT_MSG(label_name.size() <= 32,
              "Max. length of Solomon label node is 32 chars");
  stats_node[kMetadata][kMetadataSolomonLabel] = label_name;
}

void SolomonLabelValue(formats::json::ValueBuilder&& stats_node,
                       const std::string& label_name) {
  SolomonLabelValue(stats_node, label_name);
}

void SolomonChildrenAreLabelValues(formats::json::ValueBuilder& stats_node,
                                   const std::string& label_name) {
  UASSERT_MSG(label_name.size() <= 32,
              "Max. length of Solomon label node is 32 chars");
  stats_node[kMetadata][kMetadataSolomonChildrenLabels] = label_name;
}

void SolomonChildrenAreLabelValues(formats::json::ValueBuilder&& stats_node,
                                   const std::string& label_name) {
  SolomonChildrenAreLabelValues(stats_node, label_name);
}

}  // namespace statistics
}  // namespace utils
