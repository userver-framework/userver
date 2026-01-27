#include <userver/utils/statistics/request.hpp>

#include <algorithm>
#include <utility>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace {

void RemoveAddedLabels(std::vector<Label>& labels, const Request::AddLabels& add_labels) {
    labels.erase(
        std::remove_if(
            labels.begin(),
            labels.end(),
            [&add_labels](const auto& label) {
                auto it = add_labels.find(label.Name());
                return it != add_labels.end() && it->second == label.Value();
            }
        ),
        labels.end()
    );
}

}  // namespace

Request Request::MakeWithPrefix(const std::string& prefix, AddLabels add_labels, std::vector<Label> require_labels) {
    RemoveAddedLabels(require_labels, add_labels);
    return {
        prefix,
        prefix.empty() ? PrefixMatch::kNoop : PrefixMatch::kStartsWith,
        std::move(require_labels),
        std::move(add_labels)
    };
}

Request Request::MakeWithPath(const std::string& path, AddLabels add_labels, std::vector<Label> require_labels) {
    RemoveAddedLabels(require_labels, add_labels);
    UASSERT(!path.empty());
    return {path, PrefixMatch::kExact, std::move(require_labels), std::move(add_labels)};
}

Request::Request(
    std::string prefix_in,
    PrefixMatch path_match_type_in,
    std::vector<Label> require_labels_in,
    AddLabels add_labels_in
)
    : prefix(std::move(prefix_in)),
      prefix_match_type(path_match_type_in),
      require_labels(std::move(require_labels_in)),
      add_labels(std::move(add_labels_in))
{}

BaseFormatBuilder::~BaseFormatBuilder() = default;

}  // namespace utils::statistics

USERVER_NAMESPACE_END
