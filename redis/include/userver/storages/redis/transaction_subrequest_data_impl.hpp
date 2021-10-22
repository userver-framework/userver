#pragma once

#include <chrono>
#include <stdexcept>
#include <string>

#include <userver/engine/impl/blocking_future.hpp>
#include <userver/storages/redis/transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

template <typename Result, typename ReplyType>
class TransactionSubrequestDataImpl final
    : public RequestDataBase<Result, ReplyType> {
 public:
  TransactionSubrequestDataImpl(engine::impl::BlockingFuture<ReplyType> future)
      : future_(std::move(future)) {}

  void Wait() override {
    static const std::string kDescription = "Wait() for";
    CheckIsReady(kDescription);
  }

  ReplyType Get(const std::string& /*request_description*/) override {
    static const std::string kDescription = "Get()";
    CheckIsReady(kDescription);
    return future_.get();
  }

  ReplyPtr GetRaw() override {
    throw std::logic_error("call TransactionSubrequestDataImpl::GetRaw()");
  }

 private:
  void CheckIsReady(const std::string& description) {
    if (future_.wait_for(std::chrono::seconds(0)) !=
        std::future_status::ready) {
      throw NotStartedTransactionException(
          "trying to " + description +
          " transaction's subcommand result before calling Exec() + Get() for "
          "the entire transaction");
    }
  }

  engine::impl::BlockingFuture<ReplyType> future_;
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
