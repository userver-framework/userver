#include <utils/statistics/visitation.hpp>

#include <algorithm>
#include <iterator>
#include <optional>
#include <stack>
#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <boost/container/small_vector.hpp>

#include <userver/formats/common/items.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace {

template <class ContainerLeft, class ContainerRight>
bool LeftContainsRight(const ContainerLeft& left, const ContainerRight& right) {
  for (const auto& value : right) {
    if (std::find(left.begin(), left.end(), value) == left.end()) {
      return false;
    }
  }

  return true;
}

class SensorPath {
 public:
  SensorPath() = default;

  std::string_view Get() const { return path_; }

  void AppendNode(std::string_view node) {
    prefix_lengths_.push_back(path_.size());
    if (node.empty()) return;

    if (!path_.empty()) path_ += '.';
    std::replace_copy(node.begin(), node.end(), std::back_inserter(path_), '.',
                      '_');
  }

  void DiscardLastNode() {
    UASSERT(!prefix_lengths_.empty());
    UASSERT(path_.size() >= prefix_lengths_.back());
    path_.resize(prefix_lengths_.back());
    prefix_lengths_.pop_back();
  }

 private:
  std::string path_;
  std::vector<size_t> prefix_lengths_;
};

class DfsLabelsBag {
 public:
  DfsLabelsBag() = default;

  void AppendLabel(Label&& label) {
    auto& current = label_updates_.emplace();

    if (!label) return;

    for (size_t i = 0; i < labels_.size(); ++i) {
      if (labels_[i].Name() == label.Name()) {
        current.label_pos = i;
        UASSERT(!current.prev_value);
        current.prev_value = std::move(labels_[i]).Value();
        labels_[i] = std::move(label);
        return;
      }
    }

    current.label_pos = labels_.size();
    labels_.push_back(std::move(label));
  }

  void ResetLastLabel(std::string value) {
    UASSERT(!label_updates_.empty());
    const auto& label_pos = label_updates_.top().label_pos;
    UASSERT(label_pos);
    labels_[*label_pos].Value() = std::move(value);
  }

  void DiscardLastLabel() {
    auto& expired = label_updates_.top();
    if (expired.label_pos) {
      if (expired.prev_value) {
        labels_[*expired.label_pos].Value() = std::move(*expired.prev_value);
      } else {
        UASSERT(*expired.label_pos + 1 == labels_.size());
        labels_.pop_back();
      }
    }
    label_updates_.pop();
  }

  const std::vector<Label>& Labels() const { return labels_; }

 private:
  struct LabelUpdate {
    std::optional<size_t> label_pos;
    std::optional<std::string> prev_value;
  };

  std::vector<Label> labels_;
  std::stack<LabelUpdate> label_updates_;
};

struct DfsNode {
  DfsNode(const formats::json::Value& next, std::string&& path_node_,
          Label&& label_, std::string&& children_label_name_)
      : current(next),
        path_node(std::move(path_node_)),
        label(label_),
        children_label_name(std::move(children_label_name_)) {}

  bool is_in_subtree{false};
  const formats::json::Value current;
  const std::string path_node;
  Label label;
  const std::string children_label_name;
};

using DfsStack = std::stack<DfsNode>;

void ProcessInternalNode(DfsStack& dfs_stack,
                         const std::string& default_label_name,
                         const std::string& key,
                         const formats::json::Value& value) {
  std::optional<std::string> path_node;
  std::string children_label_name;

  auto label_name = default_label_name;
  auto metadata = value["$meta"];
  if (metadata.IsObject()) {
    if (metadata.HasMember("solomon_skip")) {
      path_node.emplace();
    } else if (metadata.HasMember("solomon_rename")) {
      path_node = metadata["solomon_rename"].As<std::string>();
    }

    if (metadata.HasMember("solomon_label")) {
      label_name = metadata["solomon_label"].As<std::string>();
    }

    if (metadata.HasMember("solomon_children_labels")) {
      children_label_name =
          metadata["solomon_children_labels"].As<std::string>();
    }
  }

  Label label;
  if (!label_name.empty()) {
    path_node.emplace();  // skip
    label = Label{std::move(label_name), key};
  }
  dfs_stack.emplace(value, std::move(path_node).value_or(key), std::move(label),
                    std::move(children_label_name));
}

void ProcessLeaf(BaseFormatBuilder& builder, DfsLabelsBag& labels,
                 SensorPath& path, bool has_children_label,
                 const std::string& key, const formats::json::Value& value,
                 const Request& request) {
  if (has_children_label) {
    labels.ResetLastLabel(key);
  } else {
    path.AppendNode(key);
  }
  std::optional<MetricValue> metric_value;

  if (value.IsInt64()) {
    metric_value.emplace(value.As<std::int64_t>());
  } else if (value.IsDouble()) {
    metric_value.emplace(value.As<double>());
  } else {
    // TODO remove this condition, prohibit null metrics.
    if (!value.IsNull()) {
      UASSERT_MSG(false, fmt::format("Non-numeric metric at path '{}': {}",
                                     path.Get(), ToString(value)));
    }
  }

  // The whole internal JSON metrics code is to be deprecated
  if (metric_value && utils::text::StartsWith(path.Get(), request.prefix)) {
    if (LeftContainsRight(labels.Labels(), request.require_labels)) {
      if (request.prefix_match_type != Request::PrefixMatch::kExact ||
          path.Get() == request.prefix) {
        boost::container::small_vector<LabelView, 16> labels_vector;
        labels_vector.reserve(labels.Labels().size() +
                              request.add_labels.size());
        for (const auto& l : request.add_labels) {
          labels_vector.emplace_back(l.first, l.second);
        }
        for (const auto& l : labels.Labels()) {
          labels_vector.emplace_back(l);
        }

        builder.HandleMetric(path.Get(), LabelsSpan{labels_vector},
                             *metric_value);
      }
    }
  }
  if (!has_children_label) {
    path.DiscardLastNode();
  }
}

}  // namespace

Label::Label(std::string name, std::string value)
    : name_(std::move(name)), value_(std::move(value)) {
  UASSERT(!name_.empty());
}

void VisitMetrics(BaseFormatBuilder& out,
                  const formats::json::Value& statistics_storage_json,
                  const Request& request) {
  SensorPath path;
  DfsLabelsBag labels;
  std::stack<DfsNode> dfs_stack;
  dfs_stack.emplace(statistics_storage_json, std::string{}, Label{},
                    std::string{});

  while (!dfs_stack.empty()) {
    auto& state = dfs_stack.top();
    if (state.is_in_subtree) {
      labels.DiscardLastLabel();
      path.DiscardLastNode();
      dfs_stack.pop();
      continue;
    }
    path.AppendNode(state.path_node);
    labels.AppendLabel(std::move(state.label));
    state.is_in_subtree = true;

    const bool has_children_label = !state.children_label_name.empty();
    if (has_children_label) {
      // we process them in place, without passing through stack
      labels.AppendLabel(Label{std::move(state.children_label_name), {}});
    }

    for (const auto& [key, value] : Items(state.current)) {
      if (!key.empty() && key.front() == '$') continue;

      if (value.IsObject()) {
        ProcessInternalNode(dfs_stack, state.children_label_name, key, value);
      } else {
        ProcessLeaf(out, labels, path, has_children_label, key, value, request);
      }
    }
    if (has_children_label) {
      labels.DiscardLastLabel();
    }
  }
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
