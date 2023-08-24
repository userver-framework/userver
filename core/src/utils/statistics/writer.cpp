#include <userver/utils/statistics/writer.hpp>

#include <fmt/format.h>
#include <boost/numeric/conversion/cast.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/text.hpp>

#include <utils/statistics/writer_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace {

constexpr std::string_view kFixitHint =
    "Destroy the last Writer object before using the previous one.";

template <class ContainerRight>
bool LeftContainsRight(LabelsSpan left, const ContainerRight& right) {
  for (const auto& value : right) {
    if (std::find(left.begin(), left.end(), LabelView{value}) == left.end()) {
      return false;
    }
  }

  return true;
}

bool CanSubPathSucceedExactMatch(std::string_view current_path,
                                 std::string_view required,
                                 std::size_t initial_path_size) {
  UASSERT(!required.empty());
  if (current_path.size() > required.size()) {
    return false;
  }
  UASSERT(initial_path_size <= required.size());

  if (current_path.size() == required.size()) {
    return current_path.substr(initial_path_size) ==
           required.substr(initial_path_size);
  }

  UASSERT(current_path.size() < required.size());
  return required[current_path.size()] == Writer::kDelimiter &&
         utils::text::StartsWith(required.substr(initial_path_size),
                                 current_path.substr(initial_path_size));
}

bool CanSubPathSucceedStartsWith(std::string_view current_path,
                                 std::string_view required,
                                 std::size_t initial_path_size) {
  UASSERT(!required.empty());
  if (initial_path_size >= required.size()) {
    // Already checked in a parent Writer.
    return true;
  }

  if (current_path.size() >= required.size()) {
    return utils::text::StartsWith(current_path.substr(initial_path_size),
                                   required.substr(initial_path_size));
  }

  UASSERT(current_path.size() < required.size());
  return required[current_path.size()] == Writer::kDelimiter &&
         utils::text::StartsWith(required.substr(initial_path_size),
                                 current_path.substr(initial_path_size));
}

void CheckAndWrite(impl::WriterState& state,
                   std::variant<std::int64_t, double, Rate> value) {
  UINVARIANT(!state.path.empty(),
             "Detected an attempt to write a metric by empty path");

  if (state.request.prefix_match_type != Request::PrefixMatch::kNoop) {
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

  state.builder.HandleMetric(state.path, labels, MetricValue{value});
}

}  // namespace

Writer::Writer(impl::WriterState* state) noexcept
    : state_(state),
      initial_path_size_(state ? state->path.size() : 0),
      current_path_size_(initial_path_size_),
      initial_labels_size_(state ? state->add_labels.size() : 0),
      current_labels_size_(initial_labels_size_) {}

Writer::Writer(impl::WriterState& state, LabelsSpan labels) : Writer(&state) {
  AppendLabelsSpan(labels);
}

Writer::Writer(Writer& other, MoveTag) noexcept
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

Writer Writer::operator[](std::string_view path) & { return MakeChild()[path]; }

Writer Writer::operator[](std::string_view path) && {
  UINVARIANT(!path.empty(),
             fmt::format("Detected an attempt to append an empty path to '{}'",
                         state_ ? state_->path : std::string_view{}));
  AppendPath(path);
  return MoveOut();
}

Writer Writer::MakeChild() {
  if (state_) {
    ValidateUsage();
  }
  return Writer{state_};
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

void Writer::Write(Rate value) {
  if (state_) {
    ValidateUsage();
    CheckAndWrite(*state_, value);
  }
}

void Writer::ResetState() noexcept {
  UASSERT(state_);

  UASSERT_MSG(current_path_size_ == state_->path.size(),
              fmt::format("Invalid destruction order for path '{}', expected "
                          "path size {}! {}",
                          state_->path, current_path_size_, kFixitHint));
  state_->path.resize(initial_path_size_);

  UASSERT_MSG(current_labels_size_ == state_->add_labels.size(),
              fmt::format("Invalid destruction order for path '{}', expected "
                          "labels size {} while it is {}! {}",
                          state_->path, current_labels_size_,
                          state_->add_labels.size(), kFixitHint));
  state_->add_labels.resize(initial_labels_size_);

  state_ = nullptr;
}

void Writer::ValidateUsage() {
  UASSERT(state_);

  UINVARIANT(
      current_path_size_ == state_->path.size(),
      fmt::format("Detected an attempt to simultaneously use Writer for "
                  "different paths '{}' and '{}', which is forbidden. {}",
                  state_->path, state_->path.substr(0, current_path_size_),
                  kFixitHint));

  UINVARIANT(current_labels_size_ == state_->add_labels.size(),
             fmt::format("Detected an attempt to simultaneously use Writer for "
                         "path '{}' with different labels, expected "
                         "labels size {} while it is {}! {}",
                         state_->path, current_labels_size_,
                         state_->add_labels.size(), kFixitHint));
}

void Writer::AppendPath(std::string_view path) {
  if (!state_) {
    return;
  }

  UINVARIANT(state_->path.size() + path.size() <
                 std::numeric_limits<PathSizeType>::max(),
             fmt::format("Path '{}' is too long", state_->path));

  if (!state_->path.empty()) {
    state_->path.push_back(kDelimiter);
  }
  state_->path.append(path);
  current_path_size_ = state_->path.size();

  switch (state_->request.prefix_match_type) {
    case Request::PrefixMatch::kNoop:
      break;

    case Request::PrefixMatch::kExact:
      if (!CanSubPathSucceedExactMatch(state_->path, state_->request.prefix,
                                       initial_path_size_)) {
        ResetState();
      }
      break;

    case Request::PrefixMatch::kStartsWith:
      if (!CanSubPathSucceedStartsWith(state_->path, state_->request.prefix,
                                       initial_path_size_)) {
        ResetState();
      }
      break;
  }
}

void Writer::AppendLabelsSpan(LabelsSpan labels) {
  if (!state_) {
    return;
  }

  UINVARIANT(state_->add_labels.size() + labels.size() <=
                 std::numeric_limits<LabelsSizeType>::max(),
             fmt::format("Too many labels at path '{}'", state_->path));

  state_->add_labels.insert(state_->add_labels.end(), labels.begin(),
                            labels.end());
  current_labels_size_ = state_->add_labels.size();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
