#include <userver/logging/log.hpp>
#include <utils/statistics/impl/statistics_key.hpp>

#include <boost/functional/hash.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics::impl {

StatisticsKey::StatisticsKey(std::string path, std::vector<utils::statistics::Label> labels)
    : path{std::move(path)},
      labels{std::move(labels)}
{
    label_views.reserve(this->labels.size());
    for (const auto& label : this->labels) {
        LOG_DEBUG("Creating view to {}, {}", label.Name(), label.Value());
        label_views.emplace_back(label);
    }
}

StatisticsKey StatisticsKeyView::Materialize() const {
    std::vector<utils::statistics::Label> labels;
    labels.reserve(this->labels.size());
    for (const auto& lv : this->labels) {
        labels.emplace_back(lv);
    }
    return StatisticsKey(std::string{path}, labels);
}

std::size_t StatisticsKeyHash::operator()(const StatisticsKey& key) const noexcept {
    std::size_t seed = std::hash<std::string_view>{}(key.path);
    for (const auto& label : key.labels) {
        boost::hash_combine(seed, std::hash<std::string_view>{}(label.Name()));
        boost::hash_combine(seed, std::hash<std::string_view>{}(label.Value()));
    }
    return seed;
}

std::size_t StatisticsKeyHash::operator()(const StatisticsKeyView& key) const noexcept {
    std::size_t seed = std::hash<std::string_view>{}(key.path);
    for (const auto& lv : key.labels) {
        boost::hash_combine(seed, std::hash<std::string_view>{}(lv.Name()));
        boost::hash_combine(seed, std::hash<std::string_view>{}(lv.Value()));
    }
    return seed;
}

bool StatisticsKeyEqual::operator()(const StatisticsKey& a, const StatisticsKey& b) const noexcept {
    if (a.path != b.path || a.labels.size() != b.labels.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.labels.size(); ++i) {
        if (a.labels[i] != b.labels[i]) {
            return false;
        }
    }
    return true;
}

bool StatisticsKeyEqual::operator()(const StatisticsKey& a, const StatisticsKeyView& b) const noexcept {
    if (a.path != b.path || a.labels.size() != b.labels.size()) {
        return false;
    }
    std::size_t i = 0;
    for (const auto& lv : b.labels) {
        if (utils::statistics::LabelView{a.labels[i]} != lv) {
            return false;
        }
        ++i;
    }
    return true;
}

bool StatisticsKeyEqual::operator()(const StatisticsKeyView& a, const StatisticsKey& b) const noexcept {
    return operator()(b, a);
}

}  // namespace utils::statistics::impl

USERVER_NAMESPACE_END
