#include <userver/utils/statistics/http_codes.hpp>

#include <unordered_map>

#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/statistics/metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

HttpCodes::HttpCodes(std::initializer_list<unsigned short> codes) {
  particular_codes.reserve(codes.size());
  for (auto code : codes) {
    [[maybe_unused]] auto [it, inserted] = particular_codes.emplace(code, 0);
    UASSERT(inserted);
  }
}

void HttpCodes::Account(unsigned short code) {
  if (code >= 100 && code < 200) {
    ++code_1xx;
  } else if (code >= 200 && code < 300) {
    ++code_2xx;
  } else if (code >= 300 && code < 400) {
    ++code_3xx;
  } else if (code >= 400 && code < 500) {
    ++code_4xx;
  } else if (code >= 500 && code < 600) {
    ++code_5xx;
  } else {
    ++code_other;
  }

  auto it = particular_codes.find(code);
  if (it != particular_codes.end()) {
    ++it->second;
  }
}

formats::json::Value HttpCodes::FormatReplyCodes() const {
  formats::json::ValueBuilder codes(formats::json::Type::kObject);
  codes["1xx"] = code_1xx.Load();
  codes["2xx"] = code_2xx.Load();
  codes["3xx"] = code_3xx.Load();
  codes["4xx"] = code_4xx.Load();
  codes["5xx"] = code_5xx.Load();
  codes["other"] = code_other.Load();

  for (const auto& [code, count] : particular_codes) {
    codes[std::to_string(code)] = count.Load();
  }

  utils::statistics::SolomonChildrenAreLabelValues(codes, "http_code");
  return codes.ExtractValue();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
