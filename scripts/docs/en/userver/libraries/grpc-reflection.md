## gRPC reflection

**Quality:** @ref QUALITY_TIERS "Silver Tier".

This library provides non-blocking implementation of reflection
service for gRPC in 🐙 **userver**. It is in fact an original implementation with a few modifications to
replace all standard primitives with userver ones.


## Usage

To be honest, usage is exceedingly simple:

1. Link with in your CMakeLists.txt
```cmake
find_package(userver COMPONENTS grpc grpc-reflection REQUIRED)
target_link_libraries(${PROJECT_NAME} userver::grpc-reflection)
```

2. Add component to your service
```cpp
#include <userver/grpc-reflection/reflection_service_component.hpp>


int main(int argc, char* argv[]) {
    const auto component_list = components::MinimalServerComponentList()
                                    ....
                                    .Append<grpc_reflection::ReflectionServiceComponent>()
                                    ....
}
```

3. Add simple line to your config.yaml
   Don't forget that grpc-reflection requires grpc
```
yaml
components_manager:
    components:
        grpc-reflection-service:
                # Or you can set these up in grpc-server.service-defaults
                task-processor: main-task-processor
                middlewares: []
```

4. Thats it. Reflection component will start automatically and expose every gRPC entity in your service.

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/libraries/s3api.md | @ref scripts/docs/en/userver/development/stability.md ⇨
@htmlonly </div> @endhtmlonly
