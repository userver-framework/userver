#include <userver/utest/using_namespace_userver.hpp>

#include "say_hello.hpp"

#include <fmt/format.h>

#include <userver/utils/datetime.hpp>
#include <userver/utils/datetime/timepoint_tz.hpp>

namespace samples::hello {

HelloResponseBody SayHelloTo(const HelloRequestBody& request) {
  HelloResponseBody response;

  response.text = fmt::format("Hello, {}!\n", request.name);
  response.current_time = utils::datetime::TimePointTz{utils::datetime::Now()};

  return response;
}

}  // namespace samples::hello
