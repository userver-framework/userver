#include <server/requests_view.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
const auto kDequeueBulkSize = 10000;
const auto kDequeuePollPeriod = std::chrono::milliseconds(100);
}  // namespace

namespace server {

RequestsView::RequestsView()
    : queue_(std::make_shared<Queue>()), job_requests(kDequeueBulkSize) {}

RequestsView::~RequestsView() { StopBackgroundWorker(); }

std::vector<std::shared_ptr<request::RequestBase>>
RequestsView::GetAllRequests() {
  std::vector<std::shared_ptr<request::RequestBase>> result;

  // Move all to-be-deleted items into delete_list
  // to avoid deletion under mutex.
  std::list<RequestWPtr> delete_list;

  {
    std::lock_guard<engine::Mutex> lock(requests_in_flight_mutex_);
    result.reserve(requests_in_flight_.size());

    for (auto it = requests_in_flight_.begin();
         it != requests_in_flight_.end();) {
      auto ptr = it->lock();
      if (!ptr) {
        auto old = it++;
        delete_list.splice(delete_list.begin(), requests_in_flight_, old);
        continue;
      }

      result.push_back(std::move(ptr));
      ++it;
    }
  }

  return result;
}

void RequestsView::StartBackgroundWorker() {
  job_task_ = engine::CriticalAsyncNoSpan([this]() { DoJob(); });
}

void RequestsView::StopBackgroundWorker() { job_task_.SyncCancel(); }

void RequestsView::GarbageCollect() {
  // Move all to-be-deleted items into delete_list
  // to avoid deletion under mutex.
  std::list<RequestWPtr> delete_list;

  std::lock_guard<engine::Mutex> lock(requests_in_flight_mutex_);
  for (auto it = requests_in_flight_.begin();
       it != requests_in_flight_.end();) {
    if (!it->expired()) {
      ++it;
      continue;
    }

    auto old = it++;
    delete_list.splice(delete_list.begin(), requests_in_flight_, old);
  }
}

void RequestsView::HandleQueue() {
  std::lock_guard<engine::Mutex> lock(requests_in_flight_mutex_);
  for (;;) {
    const auto count =
        queue_->try_dequeue_bulk(job_requests.begin(), job_requests.size());
    if (count == 0) break;

    for (size_t i = 0; i < count; ++i)
      requests_in_flight_.push_back(std::move(job_requests[i]));
  }
}

void RequestsView::DoJob() {
  while (!engine::current_task::ShouldCancel()) {
    GarbageCollect();
    HandleQueue();
    engine::InterruptibleSleepFor(kDequeuePollPeriod);
  }
}

}  // namespace server

USERVER_NAMESPACE_END
