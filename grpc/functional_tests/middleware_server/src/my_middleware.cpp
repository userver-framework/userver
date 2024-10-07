#include "my_middleware.hpp"

namespace functional_tests {

void MyMiddleware::CallRequestHook(const ugrpc::server::MiddlewareCallContext&,
                                   google::protobuf::Message& request) {
  auto* message = dynamic_cast<samples::api::GreetingRequest*>(&request);
  auto name = message->name();
  name += " One";
  message->set_name(name);
}

void MyMiddleware::CallResponseHook(const ugrpc::server::MiddlewareCallContext&,
                                    google::protobuf::Message& response) {
  auto* message = dynamic_cast<samples::api::GreetingResponse*>(&response);
  auto str = message->greeting();
  str += " EndOne";
  message->set_greeting(str);
}

void MyMiddleware::Handle(ugrpc::server::MiddlewareCallContext& context) const {
  context.Next();
}

std::shared_ptr<ugrpc::server::MiddlewareBase>
MyMiddlewareComponent::GetMiddleware() {
  return middleware_;
}

}  // namespace functional_tests
