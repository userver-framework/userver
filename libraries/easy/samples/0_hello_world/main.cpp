#include <userver/utest/using_namespace_userver.hpp>  // Note: this is for the purposes of samples only

#include <userver/easy.hpp>

int main(int argc, char* argv[]) {
    easy::HttpWith<>(argc, argv)
        .DefaultContentType(http::content_type::kTextPlain)
        .Route("/hello", [](const server::http::HttpRequest& /*req*/) {
            return "Hello world";  // Just return the string as a response body
        });
}
