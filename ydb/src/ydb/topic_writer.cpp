#include <userver/ydb/topic_writer.hpp>

#include <array>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <ydb/impl/topic_writer_worker.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

TopicWriter::TopicWriter(std::string name, TopicWriterSettings settings)
    : name_{std::move(name)},
      settings_{settings},
      topic_client_{std::move(settings.topic_client)}
{
    UINVARIANT(topic_client_, "topic_client is zero pointer");
    UINVARIANT(!settings.topic_name.empty(), "topic is not provided");
    UINVARIANT(settings.workers_num > 0, "workers_num is zero");
    UINVARIANT(settings.workers_num <= TopicWriterSettings::kMaxWorkers, "workers_num is too big");

    // Initialize workers
    auto factory = std::make_shared<impl::WriteSessionFactory>(topic_client_);

    workers_.reserve(settings_.workers_num);
    for (std::size_t i = 0; i < settings_.workers_num; ++i) {
        impl::TopicWriterWorkerSettings worker_settings{
            .topic = settings_.topic_name,
            .max_incoming_msg_queue_size = settings_.max_incoming_msg_queue_size,
            .max_ydb_control_event_queue_size = settings_.max_ydb_control_event_queue_size
        };

        workers_.emplace_back(std::make_shared<
                              impl::TopicWriterWorker>("worker_" + std::to_string(i), factory, worker_settings));
    }

    LOG_INFO(
        "Created YDB topic writer[{}] for topic[{}], workers[{}]",
        name_,
        settings_.topic_name,
        settings_.workers_num
    );
}

TopicWriteResult TopicWriter::WriteMessage(std::string message, const TopicWriterMetadata& meta) {
    // Simple round-robin selection of worker for now. This is unsigned type,
    // so overflow will work correctly
    auto idx = current_worker_.fetch_add(1, std::memory_order_relaxed);
    std::size_t worker_index = idx % workers_.size();

    return workers_[worker_index]->WriteMessage(std::move(message), meta);
}

void DumpMetric(utils::statistics::Writer& writer, const TopicWriter& topic_writer) {
    impl::TopicWriterWorkerStats common_stats;

    std::array<std::size_t, static_cast<std::size_t>(impl::TopicWriterWorkerState::kDoNotUse)> workers_states{};

    for (const auto& elem : topic_writer.workers_) {
        common_stats += elem->GetStatistics();

        // account worker states
        auto wstate = static_cast<std::size_t>(elem->GetState());
        workers_states[std::clamp(
            wstate,
            static_cast<std::size_t>(0),
            (static_cast<std::size_t>(impl::TopicWriterWorkerState::kDoNotUse) - 1)
        )]++;
    }

    writer
        .ValueWithLabels(common_stats, {{"topic", topic_writer.settings_.topic_name}, {"writer", topic_writer.name_}});

    {
        auto states_writer = writer["states"];
        for (std::size_t idx = 0; idx < workers_states.size(); ++idx) {
            states_writer.ValueWithLabels(
                workers_states[idx],
                {{"state", ToStringView(static_cast<impl::TopicWriterWorkerState>(idx))},
                 {"writer", topic_writer.name_},
                 {"topic", topic_writer.settings_.topic_name}}
            );
        }
    }

    if (topic_writer.settings_.stats_level == TopicWriterSettings::StatisticLevel::kWorker) {
        for (const auto& elem : topic_writer.workers_) {
            // worker will set 'topic' label itself, but 'writer' label must be set up
            // here
            writer.ValueWithLabels(*elem, {{"writer", topic_writer.name_}});
        }
    }
}

TopicWriterManager::TopicWriterManager(std::unordered_map<std::string, TopicWriterSettings> settings) {
    UASSERT_MSG(!settings.empty(), "empty list of the writers");
    for (const auto& [name, topic_writer_settings] : settings) {
        topic_writers_.emplace(name, std::make_shared<TopicWriter>(name, topic_writer_settings));
    }
}

TopicWriter& TopicWriterManager::GetTopicWriter(std::string_view writer_name) {
    auto it = topic_writers_.find(writer_name);
    if (it == topic_writers_.end()) {
        throw std::runtime_error(fmt::format("No such writer registered: {}", writer_name));
    }
    return *(it->second);
}

void DumpMetric(utils::statistics::Writer& writer, const TopicWriterManager& manager) {
    for (const auto& [name, topic_writer] : manager.topic_writers_) {
        DumpMetric(writer, *topic_writer);
    }
}

}  // namespace ydb

USERVER_NAMESPACE_END
