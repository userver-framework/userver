#pragma once

#include <userver/formats/json/inline.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <testsuite/impl/actions/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl::actions {

class CacheDumpsWrite final : public BaseTestsuiteAction {
 public:
  CacheDumpsWrite(components::TestsuiteSupport& testsuite_support)
      : testsuite_support_(testsuite_support) {}

  formats::json::Value Perform(
      const formats::json::Value& request_body) const override {
    const auto dumper_names =
        request_body["names"].As<std::vector<std::string>>();
    testsuite_support_.GetDumpControl().WriteCacheDumps(dumper_names);
    return {};
  }

 private:
  components::TestsuiteSupport& testsuite_support_;
};

class CacheDumpsRead final : public BaseTestsuiteAction {
 public:
  CacheDumpsRead(components::TestsuiteSupport& testsuite_support)
      : testsuite_support_(testsuite_support) {}

  formats::json::Value Perform(
      const formats::json::Value& request_body) const override {
    const auto dumper_names =
        request_body["names"].As<std::vector<std::string>>();
    testsuite_support_.GetDumpControl().ReadCacheDumps(dumper_names);
    return {};
  }

 private:
  components::TestsuiteSupport& testsuite_support_;
};

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
