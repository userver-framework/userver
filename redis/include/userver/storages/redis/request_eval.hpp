#pragma once

#include <userver/storages/redis/parse_reply.hpp>
#include <userver/storages/redis/reply.hpp>
#include <userver/storages/redis/request.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

template <typename ScriptResult, typename ReplyType = ScriptResult>
class [[nodiscard]] RequestEval final {
 public:
  explicit RequestEval(RequestEvalCommon&& request)
      : request_(std::move(request)) {}

  void Wait() { request_.Wait(); }

  void IgnoreResult() const { request_.IgnoreResult(); }

  ReplyType Get(const std::string& request_description = {}) {
    return ParseReply<ScriptResult, ReplyType>(request_.GetRaw(),
                                               request_description);
  }

 private:
  RequestEvalCommon request_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
