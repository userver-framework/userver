#include "hello_handler.hpp"

#include "say_hello.hpp"

namespace samples::hello {

std::string HelloHandler::
    HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext& /*request_context*/)
        const {
    // Setting Content-Type: text/plain in a microservice response ensures
    // the client interprets it as plain text, preventing misinterpretation or
    // errors. Without this header, the client might assume a different format,
    // such as JSON, HTML or XML, leading to potential processing issues or
    // incorrect handling of the data.
    request.GetHttpResponse().SetContentType(http::content_type::kTextPlain);
    return samples::hello::SayHelloTo(request.GetArg("name"));
}

}  // namespace samples::hello
