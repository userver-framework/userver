#include <engine/ev/data_pipe_to_ev.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev::impl {

void DoubleBufferingState::Reset() noexcept {
  const auto old_state = state_.Exchange(DoubleBufferFlags::kNone);
  UASSERT(!(old_state & DoubleBufferFlags::kLocked0) &&
          !(old_state & DoubleBufferFlags::kLocked1));
}

DoubleBufferingState::IndexType DoubleBufferingState::LockProducer() {
  IndexType locked_data_index = 0;

  auto old_state = state_.Load();

  std::size_t cas_count = 0;

  do {
    ++cas_count;

    if (!(old_state & DoubleBufferFlags::kLocked0)) {
      auto new_state = old_state | DoubleBufferFlags::kLocked0;
      new_state |= DoubleBufferFlags::kDataIn0;
      new_state.Clear(DoubleBufferFlags::kDataIn1);
      if (state_.CompareExchangeStrong(old_state, new_state)) {
        locked_data_index = 0;
        break;
      }
    } else {
      old_state.Clear(DoubleBufferFlags::kLocked1);
      auto new_state = old_state | DoubleBufferFlags::kLocked1;
      new_state |= DoubleBufferFlags::kDataIn1;
      new_state.Clear(DoubleBufferFlags::kDataIn0);
      if (state_.CompareExchangeStrong(old_state, new_state)) {
        locked_data_index = 1;
        break;
      }
    }
  } while (true);

  // CAS count is 4 in worst case:
  // 1) this thread loads state_ == kLocked0 | kDataIn0 | kDataIn1
  // 2) ev thread sets state_ == kDataIn1
  // 3) CAS #1 fails
  // 4) ev thread sets state_ == kLocked1 | kDataIn1
  // 5) CAS #2 fails
  // 6) ev thread sets state_ == 0
  // 7) CAS #3 fails
  // 8) CAS #4 succeeds
  UASSERT(cas_count <= 4);

  return locked_data_index;
}

void DoubleBufferingState::UnlockProducer(IndexType locked_index) noexcept {
  utils::Flags<DoubleBufferFlags> old_params;
  if (locked_index == 0) {
    old_params = state_.FetchClear(DoubleBufferFlags::kLocked0);
    UASSERT(old_params & DoubleBufferFlags::kLocked0);
  } else {
    old_params = state_.FetchClear(DoubleBufferFlags::kLocked1);
    UASSERT(old_params & DoubleBufferFlags::kLocked1);
  }

  UASSERT(!(old_params & DoubleBufferFlags::kDataIn0) ||
          !(old_params & DoubleBufferFlags::kDataIn1));
}

DoubleBufferingState::IndexType DoubleBufferingState::LockConsumer() {
  auto locked_data_index = kNoneLocked;

  auto state = state_.Load();
  if (state & DoubleBufferFlags::kDataIn0) {
    UASSERT(!(state & DoubleBufferFlags::kDataIn1));

    const auto new_state = state | DoubleBufferFlags::kLocked0;
    if (!(state & DoubleBufferFlags::kLocked0) &&
        state_.CompareExchangeStrong(state, new_state)) {
      locked_data_index = 0;
    }

    // Params are in update right now
  } else if (state & DoubleBufferFlags::kDataIn1) {
    UASSERT(!(state & DoubleBufferFlags::kDataIn0));

    const auto new_state = state | DoubleBufferFlags::kLocked1;
    if (!(state & DoubleBufferFlags::kLocked1) &&
        state_.CompareExchangeStrong(state, new_state)) {
      locked_data_index = 1;
    }

    // Params are in update right now
  } else {
    // No new params, they were grabbed by other invocation
  }

  return locked_data_index;
}

void DoubleBufferingState::UnlockConsumer(IndexType locked_index) noexcept {
  utils::Flags<DoubleBufferFlags> old_params;

  if (locked_index == 0) {
    old_params = state_.FetchClear(
        {DoubleBufferFlags::kLocked0, DoubleBufferFlags::kDataIn0});
    UASSERT(old_params & DoubleBufferFlags::kLocked0);
  } else if (locked_index == 1) {
    old_params = state_.FetchClear(
        {DoubleBufferFlags::kLocked1, DoubleBufferFlags::kDataIn1});
    UASSERT(old_params & DoubleBufferFlags::kLocked1);
  }

  UASSERT(!(old_params & DoubleBufferFlags::kDataIn0) ||
          !(old_params & DoubleBufferFlags::kDataIn1));
}

}  // namespace engine::ev::impl

USERVER_NAMESPACE_END
