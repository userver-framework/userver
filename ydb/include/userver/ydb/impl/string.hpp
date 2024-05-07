#pragma once

#include <ydb-cpp-sdk/client/types/credentials/credentials.h>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

using String =
    std::invoke_result_t<decltype(&NYdb::ICredentialsProvider::GetAuthInfo),
                         NYdb::ICredentialsProvider>;

}  // namespace ydb::impl

USERVER_NAMESPACE_END
