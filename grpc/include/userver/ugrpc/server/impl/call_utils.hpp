#pragma once

#include <userver/ugrpc/server/result.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

template <typename Call>
void Finish(Call& call, const grpc::Status& status) {
  if (status.ok()) {
    call.Finish();
  } else {
    call.FinishWithError(status);
  }
}

template <typename Call, typename Response>
void Finish(Call& call, ugrpc::server::Result<Response>&& result) {
  if (result.IsSuccess()) {
    call.Finish(std::move(result).ExtractResponse());
  } else {
    call.FinishWithError(result.GetErrorStatus());
  }
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
