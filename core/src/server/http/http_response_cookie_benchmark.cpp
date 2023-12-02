#include <benchmark/benchmark.h>

#include <fmt/compile.h>

#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/small_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const std::string kTimeFormat = "%a, %d %b %Y %H:%M:%S %Z";

std::string name_ = "name1";
std::string value_ = "value1";
std::string path_ = "path1";
std::string domain_ = "domain1";
std::string same_site_ = "same_site1";

bool secure_{true};
bool http_only_{true};
std::optional<std::chrono::system_clock::time_point> expires_{
    std::chrono::seconds{1560358305}};
std::optional<std::chrono::seconds> max_age_{std::chrono::seconds{3600}};

const int kArraySize = 1000;
}  // namespace

void http_cookie_serialization_resize_and_overwrite(benchmark::State& state) {
  USERVER_NAMESPACE::http::headers::HeadersString os;
  for ([[maybe_unused]] auto _ : state) {
    os.clear();
    os.resize_and_overwrite(
        USERVER_NAMESPACE::http::headers::kTypicalHeadersSize,
        [](char* data, std::size_t) {
          auto append = [&data](const std::string_view what) {
            std::memcpy(data, what.begin(), what.size());
            data += what.size();
          };
          char* old_data_pointer = data;
          append(name_);
          append("=");
          append(value_);
          if (!domain_.empty()) {
            append("; Domain=");
            append(domain_);
          }
          if (!path_.empty()) {
            append("; Path=");
            append(path_);
          }
          if (expires_.has_value()) {
            append("; Expires=");
            append(utils::datetime::Timestring(expires_.value(), "GMT",
                                               kTimeFormat));
          }
          if (max_age_.has_value()) {
            append("; Max-Age=");
            append(fmt::format(FMT_COMPILE("{}"), max_age_.value().count()));
          }
          if (secure_) {
            append("; Secure");
          }
          if (!same_site_.empty()) {
            append("; SameSite=");
            append(same_site_);
          }
          if (http_only_) {
            append("; HttpOnly");
          }
          return data - old_data_pointer;
        });
  }
}

void http_cookie_serialization_append(benchmark::State& state) {
  USERVER_NAMESPACE::http::headers::HeadersString os;
  for ([[maybe_unused]] auto _ : state) {
    os.clear();
    os.append(name_);
    os.append("=");
    os.append(value_);

    if (!domain_.empty()) {
      os.append("; Domain=");
      os.append(domain_);
    }
    if (!path_.empty()) {
      os.append("; Path=");
      os.append(path_);
    }
    if (expires_.has_value()) {
      os.append("; Expires=");
      os.append(
          utils::datetime::Timestring(expires_.value(), "GMT", kTimeFormat));
    }
    if (max_age_.has_value()) {
      os.append("; Max-Age=");
      os.append(fmt::format(FMT_COMPILE("{}"), max_age_.value().count()));
    }
    if (secure_) {
      os.append("; Secure");
    }
    if (!same_site_.empty()) {
      os.append("; SameSite=");
      os.append(same_site_);
    }
    if (http_only_) {
      os.append("; HttpOnly");
    }
  }
}

BENCHMARK(http_cookie_serialization_append);
BENCHMARK(http_cookie_serialization_resize_and_overwrite);

USERVER_NAMESPACE_END
