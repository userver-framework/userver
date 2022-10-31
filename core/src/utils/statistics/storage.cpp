#include <userver/utils/statistics/storage.hpp>

#include <utility>

#include <boost/container/small_vector.hpp>

#include <userver/formats/common/utils.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/text.hpp>
#include <utils/statistics/value_builder_helpers.hpp>

#include <utils/statistics/entry_impl.hpp>
#include <utils/statistics/visitation.hpp>
#include <utils/statistics/writer_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace {

const std::string kVersionField = "$version";
constexpr int kVersion = 2;

void RemoveAddedLabels(std::vector<Label>& labels,
                       const StatisticsRequest::AddLabels& add_labels) {
  labels.erase(std::remove_if(labels.begin(), labels.end(),
                              [&add_labels](const auto& label) {
                                auto it = add_labels.find(label.Name());
                                return it != add_labels.end() &&
                                       it->second == label.Value();
                              }),
               labels.end());
}

}  // namespace

StatisticsRequest StatisticsRequest::MakeWithPrefix(
    const std::string& prefix, AddLabels add_labels,
    std::vector<Label> require_labels) {
  RemoveAddedLabels(require_labels, add_labels);
  return {prefix,
          prefix.empty() ? PrefixMatch::kNoop : PrefixMatch::kStartsWith,
          std::move(require_labels), std::move(add_labels)};
}

StatisticsRequest StatisticsRequest::MakeWithPath(
    const std::string& path, AddLabels add_labels,
    std::vector<Label> require_labels) {
  RemoveAddedLabels(require_labels, add_labels);
  UASSERT(!path.empty());
  return {path, PrefixMatch::kExact, std::move(require_labels),
          std::move(add_labels)};
}

StatisticsRequest::StatisticsRequest(std::string prefix_in,
                                     PrefixMatch path_match_type_in,
                                     std::vector<Label> require_labels_in,
                                     AddLabels add_labels_in)
    : prefix(std::move(prefix_in)),
      prefix_match_type(path_match_type_in),
      require_labels(std::move(require_labels_in)),
      add_labels(std::move(add_labels_in)) {}

BaseFormatBuilder::~BaseFormatBuilder() = default;

Storage::Storage() : may_register_extenders_(true) {}

formats::json::ValueBuilder Storage::GetAsJson(
    const StatisticsRequest& request) const {
  formats::json::ValueBuilder result;
  result[kVersionField] = kVersion;

  std::shared_lock lock(mutex_);

  for (const auto& entry : metrics_sources_) {
    if (entry.writer) {
      continue;
    }

    if (request.prefix_match_type == StatisticsRequest::PrefixMatch::kNoop ||
        utils::text::StartsWith(entry.prefix_path, request.prefix) ||
        utils::text::StartsWith(request.prefix, entry.prefix_path)) {
      LOG_DEBUG() << "Getting statistics for prefix=" << entry.prefix_path;
      SetSubField(result, std::vector(entry.path_segments),
                  entry.extender(request));
    }
  }

  return result;
}

void Storage::VisitMetrics(BaseFormatBuilder& out,
                           const StatisticsRequest& request) const {
  {
    impl::WriterState state{out, request, {}, {}};
    for (const auto& [name, value] : request.add_labels) {
      state.add_labels.emplace_back(name, value);
    }

    boost::container::small_vector<LabelView, 16> labels_vector;

    std::shared_lock lock(mutex_);
    for (const auto& entry : metrics_sources_) {
      if (!entry.writer) {
        continue;
      }

      labels_vector.clear();
      labels_vector.reserve(entry.writer_labels.size());
      for (const auto& l : entry.writer_labels) {
        labels_vector.emplace_back(l.Name(), l.Value());
      }

      try {
        auto writer =
            (entry.prefix_path.empty()
                 ? Writer{state, LabelsSpan{labels_vector}}
                 : Writer{state, LabelsSpan{labels_vector}}[entry.prefix_path]);
        if (writer) {
          LOG_DEBUG() << "Getting statistics for prefix=" << entry.prefix_path;
          entry.writer(writer);
        }
      } catch (const std::exception& e) {
        UASSERT_MSG(false,
                    fmt::format("Failed to write metrics for prefix '{}': {}",
                                entry.prefix_path, e.what()));
        LOG_ERROR() << "Failed to write metrics for prefix '"
                    << entry.prefix_path << "': " << e;
      }
    }
  }

  statistics::VisitMetrics(out, GetAsJson(request).ExtractValue(), request);
}

void Storage::StopRegisteringExtenders() { may_register_extenders_ = false; }

Entry Storage::RegisterWriter(std::string prefix, WriterFunc func,
                              std::vector<Label> add_labels) {
  return DoRegisterExtender(impl::MetricsSource{
      std::move(prefix), {}, {}, std::move(func), std::move(add_labels)});
}

Entry Storage::RegisterExtender(std::string prefix, ExtenderFunc func) {
  auto prefix_split = formats::common::SplitPathString(prefix);
  return DoRegisterExtender(impl::MetricsSource{
      std::move(prefix), std::move(prefix_split), std::move(func), {}, {}});
}

Entry Storage::RegisterExtender(std::vector<std::string> prefix,
                                ExtenderFunc func) {
  auto prefix_joined = JoinPath(prefix);
  return DoRegisterExtender(impl::MetricsSource{
      std::move(prefix_joined), std::move(prefix), std::move(func), {}, {}});
}

Entry Storage::RegisterExtender(std::initializer_list<std::string> prefix,
                                ExtenderFunc func) {
  return RegisterExtender(std::vector(prefix), std::move(func));
}

Entry Storage::DoRegisterExtender(impl::MetricsSource&& source) {
  UASSERT_MSG(may_register_extenders_.load(),
              "You may not register statistics extender outside of component "
              "constructors");

  std::lock_guard lock(mutex_);
  const auto res =
      metrics_sources_.insert(metrics_sources_.end(), std::move(source));
  return Entry(Entry::Impl{this, res});
}

void Storage::UnregisterExtender(impl::StorageIterator iterator) noexcept {
  std::lock_guard lock(mutex_);
  metrics_sources_.erase(iterator);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
