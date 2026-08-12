#pragma once

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

template <typename Response>
class StreamReadFutureImplBase {
public:
    virtual ~StreamReadFutureImplBase() = default;

    virtual bool IsReady() const = 0;

    virtual bool Get() = 0;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
