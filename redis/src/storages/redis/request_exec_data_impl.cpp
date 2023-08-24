#include "request_exec_data_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

RequestExecDataImpl::RequestExecDataImpl(
    USERVER_NAMESPACE::redis::Request&& request,
    std::vector<TransactionImpl::ResultPromise>&& result_promises)
    : RequestDataImplBase(std::move(request)),
      result_promises_(std::move(result_promises)) {}

void RequestExecDataImpl::Wait() { impl::Wait(GetRequest()); }

void RequestExecDataImpl::Get(const std::string& request_description) {
  auto reply = GetReply();
  const auto& description = reply->GetRequestDescription(request_description);
  auto result = ParseReply<ReplyData>(reply, description);
  result.ExpectArray(description);
  auto& array = result.GetArray();
  if (array.size() != result_promises_.size()) {
    throw USERVER_NAMESPACE::redis::ParseReplyException(
        "unexpected received array size: got " + std::to_string(array.size()) +
        " need " + std::to_string(result_promises_.size()));
  }

  for (size_t command_idx = 0; command_idx < array.size(); command_idx++) {
    result_promises_[command_idx].ProcessReply(std::move(array[command_idx]),
                                               request_description);
  }
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
