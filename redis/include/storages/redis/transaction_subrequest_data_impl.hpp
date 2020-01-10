#pragma once

#include <chrono>
#include <stdexcept>
#include <string>

#include <engine/future.hpp>

#include <storages/redis/transaction.hpp>

namespace storages {
namespace redis {
namespace impl {

template <typename Result, typename ReplyType>
class TransactionSubrequestDataImpl final
    : public RequestDataBase<Result, ReplyType> {
 public:
  TransactionSubrequestDataImpl(engine::Future<ReplyType> future)
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

  engine::Future<ReplyType> future_;
};

}  // namespace impl
}  // namespace redis
}  // namespace storages
