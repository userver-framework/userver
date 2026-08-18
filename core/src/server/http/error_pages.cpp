#include "error_pages.hpp"

#include <stdexcept>

#include <fmt/format.h>

#include <userver/formats/parse/common_containers.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

// Only the statuses that the server reports by itself can be substituted,
// and those are always errors.
constexpr int kMinOriginalStatus = 400;
constexpr int kMaxOriginalStatus = 599;

constexpr int kMinSubstituteStatus = 100;
constexpr int kMaxSubstituteStatus = 599;

HttpStatus ParseStatus(const yaml_config::YamlConfig& value, int min_status, int max_status) {
    const auto status = value.As<int>();
    if (status < min_status || status > max_status) {
        throw std::runtime_error(fmt::format(
            "Invalid HTTP status {} in '{}': expected a value in range [{}, {}]",
            status,
            value.GetPath(),
            min_status,
            max_status
        ));
    }
    return static_cast<HttpStatus>(status);
}

std::optional<std::string> ParseBody(const yaml_config::YamlConfig& page) {
    auto body = page["body"].As<std::optional<std::string>>();
    const auto body_path = page["body-path"].As<std::optional<std::string>>();

    if (body && body_path) {
        throw std::runtime_error(
            fmt::format("Both 'body' and 'body-path' are set in '{}', remove one of them", page.GetPath())
        );
    }
    if (!body_path) {
        return body;
    }

    try {
        return fs::blocking::ReadFileContents(*body_path);
    } catch (const std::exception& ex) {
        throw std::runtime_error(fmt::format(
            "Failed to read the error page body from the file '{}' set in '{}': {}",
            *body_path,
            page["body-path"].GetPath(),
            ex.what()
        ));
    }
}

std::vector<std::pair<std::string, std::string>> ParseHeaders(const yaml_config::YamlConfig& headers) {
    std::vector<std::pair<std::string, std::string>> result;
    if (headers.IsMissing() || headers.IsNull()) {
        return result;
    }

    for (const auto& [name, value] : yaml_config::Items(headers)) {
        result.emplace_back(name, value.As<std::string>());
    }
    return result;
}

}  // namespace

ErrorPages::ErrorPages(std::unordered_map<HttpStatus, ErrorPage> pages)
    : pages_(std::move(pages))
{}

const ErrorPage* ErrorPages::Find(HttpStatus status) const noexcept { return utils::FindOrNullptr(pages_, status); }

void ApplyErrorPage(const ErrorPage& page, HttpResponse& response) {
    if (page.status) {
        response.SetStatus(*page.status);
    }
    if (page.body) {
        response.SetData(*page.body);
    }
    for (const auto& [name, value] : page.headers) {
        response.SetHeader(name, value);
    }
}

ErrorPages Parse(const yaml_config::YamlConfig& value, formats::parse::To<ErrorPages>) {
    std::unordered_map<HttpStatus, ErrorPage> pages;

    for (const auto& item : value) {
        ErrorPage page;
        if (const auto& status = item["status"]; !status.IsMissing()) {
            page.status = ParseStatus(status, kMinSubstituteStatus, kMaxSubstituteStatus);
        }
        page.body = ParseBody(item);
        page.headers = ParseHeaders(item["headers"]);

        if (!page.status && !page.body && page.headers.empty()) {
            throw std::runtime_error(fmt::format(
                "Error page '{}' would change nothing, set at least one of "
                "'status', 'body', 'body-path', 'headers'",
                item.GetPath()
            ));
        }

        const auto& statuses = item["statuses"];
        if (statuses.IsMissing() || statuses.GetSize() == 0) {
            throw std::runtime_error(fmt::format(
                "No 'statuses' to substitute the response for are set in the error page '{}'", item.GetPath()
            ));
        }

        for (const auto& status : statuses) {
            const auto original_status = ParseStatus(status, kMinOriginalStatus, kMaxOriginalStatus);
            if (!pages.emplace(original_status, page).second) {
                throw std::runtime_error(fmt::format(
                    "HTTP status {} is set in more than one error page, the last one is '{}'",
                    static_cast<int>(original_status),
                    item.GetPath()
                ));
            }
        }
    }

    return ErrorPages{std::move(pages)};
}

}  // namespace server::http

USERVER_NAMESPACE_END
