#pragma once

#include <algorithm>
#include <cstddef>
#include <mutex>
#include <span>

#include <boost/intrusive/slist.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::fast {

class GlobalIntrusiveQueue final {
public:
    using TaskContextList = boost::intrusive::slist<
        impl::TaskContext,
        boost::intrusive::base_hook<impl::TaskContextGlobalQueueHook>,
        boost::intrusive::constant_time_size<true>,
        boost::intrusive::cache_last<true>>;

    GlobalIntrusiveQueue() = default;

    void Push(boost::intrusive_ptr<impl::TaskContext>&& context) {
        UASSERT(context);
        impl::TaskContext* const task = context.detach();

        const std::lock_guard lock{mutex_};
        queue_.push_back(*task);
    }

    void Offload(std::span<impl::TaskContext* const> buffer) {
        if (buffer.empty()) {
            return;
        }

        TaskContextList batch;
        for (impl::TaskContext* task : buffer) {
            batch.push_back(*task);
        }

        const std::lock_guard lock{mutex_};
        queue_.splice_after(LastPosition(queue_), batch);
    }

    boost::intrusive_ptr<impl::TaskContext> TryPop() noexcept {
        const std::lock_guard lock{mutex_};
        if (queue_.empty()) {
            return nullptr;
        }

        impl::TaskContext& task = queue_.front();
        queue_.pop_front();
        return boost::intrusive_ptr<impl::TaskContext>{&task, /* add_ref= */ false};
    }

    std::size_t Grab(std::span<impl::TaskContext*> buffer, std::size_t workers) noexcept {
        const std::lock_guard lock{mutex_};
        const std::size_t size = queue_.size();
        if (size == 0) {
            return 0;
        }

        const std::size_t share = std::max(size / workers, std::size_t{1});
        const std::size_t to_grab = std::min(buffer.size(), share);

        std::size_t count = 0;
        while (count < to_grab && !queue_.empty()) {
            buffer[count++] = &queue_.front();
            queue_.pop_front();
        }
        return count;
    }

    std::size_t GetSize() const noexcept {
        const std::lock_guard lock{mutex_};
        return queue_.size();
    }

private:
    static TaskContextList::const_iterator LastPosition(TaskContextList& list) noexcept {
        return list.previous(list.end());
    }

    mutable std::mutex mutex_;
    TaskContextList queue_;
};

}  // namespace engine::fast

USERVER_NAMESPACE_END
