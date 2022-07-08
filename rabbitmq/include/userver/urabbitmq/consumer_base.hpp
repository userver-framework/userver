#pragma once

#include <memory>

#include <userver/utils/periodic_task.hpp>

#include <userver/urabbitmq/typedefs.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class Cluster;
class ConsumerBaseImpl;

/// Base class for your consumers.
///
/// You should derive from it and override `Process` method, which gets called
/// when a new message arrives from the broker.
///
/// Note that since `Process` is pure virtual in the base class you must
/// call `Stop` in the derived class
/// (either in destructor or whenever pleases you), otherwise you might
/// end up in a situation when pure virtual method is called, which BOOMs.
///
/// Library guarantees `at least once` delivery, hence some deduplication might
/// be needed ou your side.
class ConsumerBase {
 public:
  ConsumerBase(std::shared_ptr<Cluster> cluster, const Queue& queue);
  virtual ~ConsumerBase();

  /// Call this method to start consuming messages from the broker.
  void Start();

  /// Call this method to stop consuming messages from the broker.
  /// You must call it before your derived class is destroyed,
  /// otherwise it's UB.
  void Stop();

 protected:
  /// Override this method in derived class and implement
  /// message handling logic.
  ///
  /// If this method returns successfully message would be acked (best effort)
  /// to the broker, if this method throws the message would be requeued.
  ///
  /// Please keep in mind that it is possible for the message to be delivered
  /// again even if `Process` returns successfully: sadly we can't guarantee
  /// that `ack` ever reached the broker (network issues or unexpected shutdown,
  /// for example).
  /// It is however guaranteed for message to be requeued if `Process` fails.
  virtual void Process(std::string message) = 0;

 private:
  std::shared_ptr<Cluster> cluster_;

  const std::string queue_name_;
  std::unique_ptr<ConsumerBaseImpl> impl_;
  utils::PeriodicTask monitor_{};
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
