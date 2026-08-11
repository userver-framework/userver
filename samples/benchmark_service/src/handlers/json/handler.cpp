#include "handler.hpp"
#include <userver/utest/using_namespace_userver.hpp>

#include <charconv>
#include <string>

#include <userver/components/component_context.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/utils/from_string.hpp>

#include <schemas/types.hpp>

#include "dataset_provider.hpp"

namespace userver_httparena::json {
Handler::Handler(const components::ComponentConfig& config, const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      dataset_provider_{context.FindComponent<DatasetProvider>()}
{}

std::string Handler::HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
    const {
    const auto& count_str = request.GetPathArg("count");
    const auto& m_str = request.GetArg("m");

    auto count = 0;
    std::from_chars(count_str.data(), count_str.data() + count_str.size(), count);
    auto m = 1.0;
    if (!m_str.empty()) {
        m = utils::FromString<double>(m_str);
    }

    const auto& items = dataset_provider_.GetItems();
    if (count < 0) {
        count = 0;
    }
    if (static_cast<size_t>(count) > items.size()) {
        count = static_cast<int>(items.size());
    }

    JsonResponse resp;
    resp.count = count;
    resp.items.assign(items.begin(), items.begin() + count);
    for (auto& ri : resp.items) {
        ri.total = static_cast<double>(ri.price) * ri.quantity * m;
    }

    request.GetHttpResponse().SetHeader(http::headers::kContentType, "application/json");
    return ToJsonString(resp);
}
}  // namespace userver_httparena::json
