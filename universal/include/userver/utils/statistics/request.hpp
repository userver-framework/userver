#pragma once

/// @file userver/utils/statistics/request.hpp
/// @brief @copybrief utils::statistics::Request

#include <string>
#include <unordered_map>
#include <vector>

#include <userver/utils/statistics/labels.hpp>
#include <userver/utils/statistics/metric_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @ingroup userver_universal
///
/// @brief Class describing the request for metrics data.
///
/// Metric path and metric name are the same thing. For example for code like
///
/// @code
/// writer["a"]["b"]["c"] = 42;
/// @endcode
///
/// the metric name is "a.b.c" and in Prometheus format it would be escaped like
/// "a_b_c{} 42".
///
/// @note `add_labels` should not match labels from Storage, otherwise the
/// returned metrics may not be read by metrics server.
class Request final {
public:
    using AddLabels = std::unordered_map<std::string, std::string>;

    /// Default request without parameters. Equivalent to requesting all the
    /// metrics without adding or requiring any labels.
    Request() = default;

    /// Makes request for metrics whose path starts with the `prefix`.
    static Request MakeWithPrefix(
        const std::string& prefix,
        AddLabels add_labels = {},
        std::vector<Label> require_labels = {}
    );

    /// Makes request for metrics whose path is `path`.
    static Request MakeWithPath(
        const std::string& path,
        AddLabels add_labels = {},
        std::vector<Label> require_labels = {}
    );

    /// Return metrics whose path matches with this `prefix`
    const std::string prefix{};

    /// Enum for different match types of the `prefix`
    enum class PrefixMatch {
        kNoop,        ///< Do not match, the `prefix` is empty
        kExact,       ///< `prefix` equal to path
        kStartsWith,  ///< Metric path starts with `prefix`
    };

    /// Match type of the `prefix`
    const PrefixMatch prefix_match_type = PrefixMatch::kNoop;

    /// Require those labels in the metric
    const std::vector<Label> require_labels{};

    /// Add those labels to each returned metric
    const AddLabels add_labels{};

private:
    Request(
        std::string prefix_in,
        PrefixMatch path_match_type_in,
        std::vector<Label> require_labels_in,
        AddLabels add_labels_in
    );
};

class BaseFormatBuilder {
public:
    virtual ~BaseFormatBuilder();

    virtual void HandleMetric(std::string_view path, LabelsSpan labels, const MetricValue& value) = 0;
};

}  // namespace utils::statistics

USERVER_NAMESPACE_END
