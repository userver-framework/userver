#include <iostream>
#include <string>

#include <userver/formats/json.hpp>

int main() {
  auto json = userver::formats::json::FromString(R"({
    "test": "hello from universal"
  })");
  std::cout << userver::formats::json::ToString(json) << std::endl;
}
