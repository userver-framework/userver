#pragma once

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

class Writer;

}  // namespace utils::statistics

namespace utils::statistics::impl {

class MetricWrapperBase {
public:
    MetricWrapperBase& operator=(MetricWrapperBase&&) = delete;
    virtual ~MetricWrapperBase();

    virtual void DumpToWriter(Writer& writer) = 0;

    virtual void Reset() = 0;
};

template <typename Metric>
class MetricWrapper : public MetricWrapperBase {
public:
    template <typename... Args>
    explicit MetricWrapper(const Args&... args)
        : data_(args...)
    {}

    MetricWrapper(const MetricWrapper&) = delete;
    MetricWrapper(MetricWrapper&&) = delete;

    Metric& Get() { return data_; }

private:
    Metric data_;
};

}  // namespace utils::statistics::impl

USERVER_NAMESPACE_END
