#pragma once

#include <list>
#include <memory>

#include <moodycamel/concurrentqueue.h>
#include <userver/engine/mutex.hpp>
#include <userver/server/request/request_base.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {

class RequestsView final {
 public:
  RequestsView();
  ~RequestsView();

  using RequestWPtr = std::weak_ptr<request::RequestBase>;
  using Queue = moodycamel::ConcurrentQueue<RequestWPtr>;

  std::shared_ptr<Queue> GetQueue() { return queue_; }

  std::vector<std::shared_ptr<request::RequestBase>> GetAllRequests();

  void StartBackgroundWorker();

  void StopBackgroundWorker();

 private:
  void DoJob();

  void GarbageCollect();

  void HandleQueue();

  std::shared_ptr<Queue> queue_;
  engine::TaskWithResult<void> job_task_;
  std::vector<RequestWPtr> job_requests;

  engine::Mutex requests_in_flight_mutex_;
  std::list<RequestWPtr> requests_in_flight_;
};

}  // namespace server

USERVER_NAMESPACE_END
