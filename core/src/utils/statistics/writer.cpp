#include <userver/utils/statistics/writer.hpp>

#include <boost/container/small_vector.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/text.hpp>

#include <utils/statistics/writer_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace {

template <class ContainerRight>
bool LeftContainsRight(LabelsSpan left, const ContainerRight& right) {
  for (const auto& value : right) {
    if (std::find(left.begin(), left.end(), LabelView{value}) == left.end()) {
      return false;
    }
  }

  return true;
}

bool IsExactMatchWouldFail(std::string_view current_path,
                           std::string_view required) {
  UASSERT(!required.empty());
  if (current_path.size() > required.size()) {
    return true;
  }

  if (current_path.size() == required.size()) {
    return current_path != required;
  }

  UASSERT(current_path.size() < required.size());
  if (required[current_path.size()] != Writer::kDelimiter) {
    return true;
  }

  return !utils::text::StartsWith(required, current_path);
}

bool IsStartsWithMatchWouldFail(std::string_view current_path,
                                std::string_view required) {
  UASSERT(!required.empty());
  if (current_path.size() >= required.size()) {
    return !utils::text::StartsWith(current_path, required);
  }

  UASSERT(current_path.size() < required.size());
  return required[current_path.size()] != Writer::kDelimiter ||
         !utils::text::StartsWith(required, current_path);
}

void CheckAndWrite(impl::WriterState& state,
                   std::variant<std::int64_t, double> value) {
  UINVARIANT(!state.path.empty(),
             "Detected an attempt to write a metric by empty path");

  if (state.request.prefix_match_type !=
      StatisticsRequest::PrefixMatch::kNoop) {
    UASSERT(!state.request.prefix.empty());
    if (state.path.size() < state.request.prefix.size()) {
      return;
    } else {
      // Already checked in Writer constructor and is OK
    }
  }

  const LabelsSpan labels{state.add_labels};
  if (!LeftContainsRight(labels, state.request.require_labels)) {
    return;
  }

  state.builder.HandleMetric(state.path, labels, value);
}

}  // namespace

LabelView::LabelView(const Label& label) noexcept
    : name_(label.Name()), value_(label.Value()) {}

LabelsSpan::LabelsSpan(const LabelView* begin, const LabelView* end) noexcept
    : begin_(begin), end_(end) {
  UASSERT(begin <= end);
}

bool operator<(const LabelView& x, const LabelView& y) noexcept {
  return x.Name() < y.Name() || (x.Name() == y.Name() && x.Value() < y.Value());
}

bool operator==(const LabelView& x, const LabelView& y) noexcept {
  return x.Name() == y.Name() && x.Value() == y.Value();
}

BaseFormatBuilder::~BaseFormatBuilder() = default;

Writer::Writer(impl::WriterState* state, MarkDestructionReady) noexcept
    : state_(state),
      initial_path_size_(state_ ? state_->path.size() : 0),
      current_path_size_(initial_path_size_),
      initial_labels_size_(state_ ? state_->add_labels.size() : 0),
      current_labels_size_(initial_labels_size_) {}

Writer::Writer(impl::WriterState* state, std::string_view path,
               LabelsSpan labels)
    : Writer(state, MarkDestructionReady{}) {
  if (!state_) {
    return;
  }

  state_->add_labels.insert(state_->add_labels.end(), labels.begin(),
                            labels.end());
  UINVARIANT(
      state_->add_labels.size() <= std::numeric_limits<LabelsSizeType>::max(),
      "Too many labels");
  current_labels_size_ = state_->add_labels.size();

  if (path.empty()) {
    return;
  }

  if (!state_->path.empty()) {
    state_->path.push_back(kDelimiter);
  }
  state_->path.append(path);
  UINVARIANT(state_->path.size() <= std::numeric_limits<PathSizeType>::max(),
             "Path is too long");
  current_path_size_ = state_->path.size();

  switch (state_->request.prefix_match_type) {
    case StatisticsRequest::PrefixMatch::kNoop:
      break;

    case StatisticsRequest::PrefixMatch::kExact:
      UASSERT(!state_->request.prefix.empty());
      if (IsExactMatchWouldFail(state_->path, state_->request.prefix)) {
        ResetState();
      }
      break;

    case StatisticsRequest::PrefixMatch::kStartsWith:
      UASSERT(!state_->request.prefix.empty());
      if (initial_path_size_ < state_->request.prefix.size()) {
        if (IsStartsWithMatchWouldFail(state_->path, state_->request.prefix)) {
          ResetState();
        }
      }
      break;
  }
}

Writer::Writer(Writer&& other) noexcept
    : state_(other.state_),
      initial_path_size_(other.initial_path_size_),
      current_path_size_(other.current_path_size_),
      initial_labels_size_(other.initial_labels_size_),
      current_labels_size_(other.current_labels_size_) {
  other.state_ = nullptr;
}

Writer::~Writer() {
  if (state_) {
    ResetState();
  }
}

Writer Writer::operator[](std::string_view path) {
  UINVARIANT(!path.empty(),
             fmt::format("Detected an attempt to append an empty path to '{}'",
                         state_->path));

  UINVARIANT(!state_ || current_path_size_ == state_->path.size(),
             fmt::format("Detected an attempt to simultaneously use Writer for "
                         "different paths '{}' and '{}.{}', which is forbidden",
                         state_->path,
                         state_->path.substr(0, current_path_size_), path));
  return Writer{state_, path, {}};
}

void Writer::Write(unsigned long long value) {
  if (state_) {
    ValidateUsage();
    CheckAndWrite(*state_, boost::numeric_cast<std::int64_t>(value));
  }
}

void Writer::Write(long long value) {
  if (state_) {
    ValidateUsage();
    CheckAndWrite(*state_, static_cast<std::int64_t>(value));
  }
}

void Writer::Write(double value) {
  if (state_) {
    ValidateUsage();
    CheckAndWrite(*state_, value);
  }
}

void Writer::ResetState() noexcept {
  UASSERT(state_);

  UASSERT_MSG(current_path_size_ == state_->path.size(),
              "Invalid destruction order! Writer should not be stored!");
  state_->path.resize(initial_path_size_);

  UASSERT_MSG(current_labels_size_ == state_->add_labels.size(),
              "Invalid destruction order! Writer should not be stored!");
  state_->add_labels.resize(initial_labels_size_);

  state_ = nullptr;
}

void Writer::ValidateUsage() {
  UASSERT(state_);

  UINVARIANT(
      current_path_size_ == state_->path.size(),
      fmt::format("Detected an attempt to simultaneously use Writer for "
                  "different paths '{}' and '{}', which is forbidden",
                  state_->path, state_->path.substr(0, current_path_size_)));
  UASSERT(current_labels_size_ == state_->add_labels.size());
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
