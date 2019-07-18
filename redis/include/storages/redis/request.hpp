#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/optional.hpp>

#include <utils/clang_format_workarounds.hpp>

#include <storages/redis/reply_types.hpp>
#include <storages/redis/request_data_base.hpp>

namespace storages {
namespace redis {

template <typename Result, typename ReplyType = Result>
class USERVER_NODISCARD Request final {
 public:
  explicit Request(std::unique_ptr<RequestDataBase<Result, ReplyType>>&& impl)
      : impl_(std::move(impl)) {}

  void Wait() { impl_->Wait(); }

  ReplyType Get(const std::string& request_description = {}) {
    return impl_->Get(request_description);
  }

 private:
  std::unique_ptr<RequestDataBase<Result, ReplyType>> impl_;
};

using RequestAppend = Request<size_t>;
using RequestDel = Request<size_t>;
using RequestGet = Request<boost::optional<std::string>>;
using RequestHdel = Request<size_t>;
using RequestHget = Request<boost::optional<std::string>>;
using RequestHgetall = Request<std::unordered_map<std::string, std::string>>;
using RequestHmset = Request<StatusOk, void>;
using RequestHset = Request<HsetReply>;
using RequestKeys = Request<std::vector<std::string>>;
using RequestMget = Request<std::vector<boost::optional<std::string>>>;
using RequestPing = Request<StatusPong, void>;
using RequestPingMessage = Request<std::string>;
using RequestPublish = Request<size_t>;
using RequestSet = Request<StatusOk, void>;
using RequestSetOptions = Request<SetReply>;
using RequestSetex = Request<StatusOk, void>;
using RequestStrlen = Request<size_t>;
using RequestTtl = Request<TtlReply>;
using RequestType = Request<TypeReply>;
using RequestZadd = Request<size_t>;
using RequestZaddIncr = Request<std::string>;
using RequestZaddIncrExist = Request<boost::optional<std::string>>;
using RequestZrangebyscore = Request<std::vector<std::string>>;
using RequestZrangebyscoreWithScores = Request<std::vector<MemberScore>>;
using RequestZrem = Request<size_t>;
using RequestZscore = Request<boost::optional<double>>;

}  // namespace redis
}  // namespace storages
