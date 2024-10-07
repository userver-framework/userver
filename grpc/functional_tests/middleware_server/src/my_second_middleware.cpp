#include "my_second_middleware.hpp"

namespace functional_tests {

void MySecondMiddleware::CallRequestHook(
    const ugrpc::server::MiddlewareCallContext&,
    google::protobuf::Message& request) {
  auto* message = dynamic_cast<samples::api::GreetingRequest*>(&request);
  auto name = message->name();
  name += " Two";
  message->set_name(name);
}

void MySecondMiddleware::CallResponseHook(
    const ugrpc::server::MiddlewareCallContext&,
    google::protobuf::Message& response) {
  auto* message = dynamic_cast<samples::api::GreetingResponse*>(&response);
  auto str = message->greeting();
  str += " EndTwo";
  message->set_greeting(str);
}

void MySecondMiddleware::Handle(
    ugrpc::server::MiddlewareCallContext& context) const {
  context.Next();
}

std::shared_ptr<ugrpc::server::MiddlewareBase>
MySecondMiddlewareComponent::GetMiddleware() {
  return middleware_;
}

}  // namespace functional_tests
