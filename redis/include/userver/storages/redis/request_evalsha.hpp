#pragma once

#include <boost/algorithm/string/predicate.hpp>
#include <userver/storages/redis/parse_reply.hpp>
#include <userver/storages/redis/reply.hpp>
#include <userver/storages/redis/request.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

template <typename ScriptResult, typename ReplyType = ScriptResult>
class [[nodiscard]] RequestEvalSha final {
 public:
  class EvalShaResult final {
   public:
    bool IsNoScriptError() const { return no_script_error_; }
    bool HasValue() const { return reply_.has_value(); }
    const ReplyType& Get() const { return *reply_; }
    ReplyType Extract() { return std::move(*reply_); }

   private:
    friend class RequestEvalSha;
    EvalShaResult() = default;
    explicit EvalShaResult(bool no_script) : no_script_error_{no_script} {}
    EvalShaResult(ReplyType&& reply) : reply_(std::move(reply)) {}
    std::optional<ReplyType> reply_;
    bool no_script_error_ = false;
  };

  explicit RequestEvalSha(RequestEvalShaCommon&& request)
      : request_(std::move(request)) {}

  void Wait() { request_.Wait(); }

  void IgnoreResult() const { request_.IgnoreResult(); }

  EvalShaResult Get(const std::string& request_description = {}) {
    auto reply_ptr = request_.GetRaw();
    const auto& reply_data = reply_ptr->data;
    if (reply_data.IsError()) {
      const auto& msg = reply_data.GetError();
      if (boost::starts_with(msg, "NOSCRIPT")) {
        return EvalShaResult(true);
      }
    }
    /// no error try treat as usual eval
    return ParseReply<ScriptResult, ReplyType>(std::move(reply_ptr),
                                               request_description);
  }

 private:
  RequestEvalShaCommon request_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
