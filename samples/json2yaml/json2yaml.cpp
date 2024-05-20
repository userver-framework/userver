/// [json2yaml - main]
#include "json2yaml.hpp"

#include <iostream>

int main() {
  namespace formats = USERVER_NAMESPACE::formats;

  auto json = formats::json::FromStream(std::cin);
  formats::yaml::Serialize(json.ConvertTo<formats::yaml::Value>(), std::cout);
  std::cout << std::endl;
}
/// [json2yaml - main]
