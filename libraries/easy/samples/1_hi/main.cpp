#include <userver/utest/using_namespace_userver.hpp>  // Note: this is for the purposes of samples only

#include <userver/easy.hpp>

std::string Greet(const server::http::HttpRequest& req) {
    const auto& username = req.GetPathArg("user");
    return "Hello, " + username;
}

struct Hi {
    std::string operator()(const server::http::HttpRequest& req) const { return "Hi, " + req.GetArg("name"); }
};

int main(int argc, char* argv[]) {
    easy::HttpWith<>(argc, argv)
        .DefaultContentType(http::content_type::kTextPlain)
        .Get("/hi/{user}", &Greet)
        .Get("/hi", Hi{});
}
