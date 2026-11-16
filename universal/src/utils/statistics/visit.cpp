#include <userver/utils/statistics/visit.hpp>

#include <ranges>
#include <vector>

#include <userver/utils/algo.hpp>

#include <utils/statistics/writer_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

void VisitMetrics(WriterFuncRef func, BaseFormatBuilder& out, const Request& request) {
    const auto labels =
        request.add_labels |
        std::views::transform([](const auto& label) { return LabelView{label.first, label.second}; });
    impl::WriterState state{
        .builder = out,
        .request = request,
        .path = {},
        .add_labels = utils::AsContainer<std::vector<LabelView>>(labels),
    };
    Writer writer{&state};
    func(writer);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
