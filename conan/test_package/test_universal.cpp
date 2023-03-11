#include <iostream>
#include <string>

#include <userver/formats/json.hpp>

int main() {
  auto json = USERVER_NAMESPACE::formats::json::FromString(R"({
    "test": "hello from universal"
  })");
  std::cout << USERVER_NAMESPACE::formats::json::ToString(json) << std::endl;
}
