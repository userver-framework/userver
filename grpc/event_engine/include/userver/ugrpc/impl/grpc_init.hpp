#pragma once

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

// Prepares the EventEngine runtime and owns one gRPC initialization reference.
// Construct the first instance before gRPC initialization or experiment lookup.
// Successful runtime setup is shared by later instances; each holds its own library reference.
class GrpcInit final {
public:
    GrpcInit();

    GrpcInit(const GrpcInit&) = delete;
    GrpcInit& operator=(const GrpcInit&) = delete;
    ~GrpcInit();
};

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END
