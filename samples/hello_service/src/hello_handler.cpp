#include "hello_handler.hpp"

#include "say_hello.hpp"

namespace samples::hello {

std::string HelloHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext& /*request_context*/) const {
  return SayHelloTo(request.GetArg("name"));
}

}  // namespace samples::hello
