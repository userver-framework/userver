#include <storages/odbc/detail/tracing.hpp>

#include <algorithm>
#include <cctype>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail::tracing {

namespace {

bool IsWordBorder(char c) { return !std::isalnum(static_cast<unsigned char>(c)) && c != '"' && c != '_' && c != '-'; }

std::string_view FindCommandName(std::string_view str) {
    const std::size_t max_search_depth = std::min(std::size_t{128}, str.size());
    const auto end_it = str.begin() + max_search_depth;

    const auto start = std::find_if_not(str.begin(), end_it, IsWordBorder);
    const auto end = std::find_if(start, end_it, IsWordBorder);
    if (start == end) return {};

    const std::string_view command{start, static_cast<std::size_t>(end - start)};

    auto ieq = [](std::string_view a, std::string_view b) {
        if (a.size() != b.size()) return false;
        for (std::size_t i = 0; i < a.size(); ++i) {
            const auto ca = static_cast<unsigned char>(a[i]);
            const auto cb = static_cast<unsigned char>(b[i]);
            if (std::tolower(ca) != std::tolower(cb)) return false;
        }
        return true;
    };

    // keep list small; unknown commands fall back to generic span name
    if (ieq(command, "select")) return "select";
    if (ieq(command, "insert")) return "insert";
    if (ieq(command, "update")) return "update";
    if (ieq(command, "delete")) return "delete";
    if (ieq(command, "with")) return "with";
    if (ieq(command, "begin")) return "begin";
    if (ieq(command, "commit")) return "commit";
    if (ieq(command, "rollback")) return "rollback";
    if (ieq(command, "create")) return "create";
    if (ieq(command, "alter")) return "alter";

    return {};
}

}  // namespace

std::string MakeQuerySpanName(std::string_view statement) {
    if (const auto cmd = FindCommandName(statement); !cmd.empty()) {
        return fmt::format("odbc_query {}", cmd);
    }
    return "odbc_query";
}

}  // namespace storages::odbc::detail::tracing

USERVER_NAMESPACE_END

