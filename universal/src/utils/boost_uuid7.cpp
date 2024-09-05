#include <userver/utils/boost_uuid7.hpp>

#include <chrono>
#include <limits>

#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>

#include <userver/compiler/thread_local.hpp>
#include <userver/utils/datetime/wall_coarse_clock.hpp>
#include <userver/utils/rand.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

/// Implementation is based on PostgreSQL
/// https://commitfest.postgresql.org/43/4388/
class UuidV7Generator {
 public:
  UuidV7Generator(utils::RandomBase& rng)
      : random_generator_(&rng,
                          boost::uniform_int<std::uint64_t>(
                              std::numeric_limits<std::uint64_t>::min(),
                              std::numeric_limits<std::uint64_t>::max())) {}

  boost::uuids::uuid operator()() {
    boost::uuids::uuid uuid{};
    auto current_timestamp = CurrentUnixTimestamp();

    if (current_timestamp <= previous_timestamp_) {
      ++sequence_counter_;

      if (sequence_counter_ > kMaxSequenceCounterValue) {
        // In order to protect from rollover we will increment
        // timestamp ahead of the actual time.
        // See section `Counter Rollover Handling`
        // https://datatracker.ietf.org/doc/html/draft-ietf-uuidrev-rfc4122bis-09#monotonicity_counters

        sequence_counter_ = 0;
        ++previous_timestamp_;
      }

      // Protection from leaping backward
      current_timestamp = previous_timestamp_;

      // Fill var and rand_b with random data
      GenerateRandomBlock(utils::span<std::uint8_t>(uuid.data).subspan(8));

      // Fill rand_a and rand_b with counter data

      // 4 most significant bits of 18-bit counter
      uuid.data[6] = static_cast<std::uint8_t>(sequence_counter_ >> 18);
      // next 8 bits
      uuid.data[7] = static_cast<std::uint8_t>(sequence_counter_ >> 10);
      // next 6 bits (2 most significant will be overwritten with var)
      uuid.data[8] = static_cast<std::uint8_t>(sequence_counter_ >> 4);
      // 4 least significant bits
      uuid.data[9] = (uuid.data[9] & 0xF) |
                     static_cast<std::uint8_t>(sequence_counter_ << 4);
    } else {
      // fill ver, rand_a, var and rand_b with random data
      GenerateRandomBlock(utils::span<std::uint8_t>(uuid.data).subspan(6));

      // Keep most significant bit of a counter initialized as zero
      // for guarding against counter rollover.
      // See section `Fixed-Length Dedicated Counter Seeding`
      // https://datatracker.ietf.org/doc/html/draft-ietf-uuidrev-rfc4122bis-09#monotonicity_counters
      uuid.data[6] &= 0xF7;

      sequence_counter_ =
          (static_cast<std::uint32_t>(uuid.data[6] & 0x0F) << 18) +
          (static_cast<std::uint32_t>(uuid.data[7]) << 10) +
          (static_cast<std::uint32_t>(uuid.data[8] & 0x3F) << 4) +
          (static_cast<std::uint32_t>(uuid.data[9]) >> 4);
      previous_timestamp_ = current_timestamp;
    }

    // Fill unix_ts_ms
    uuid.data[0] = static_cast<std::uint8_t>(current_timestamp >> 40);
    uuid.data[1] = static_cast<std::uint8_t>(current_timestamp >> 32);
    uuid.data[2] = static_cast<std::uint8_t>(current_timestamp >> 24);
    uuid.data[3] = static_cast<std::uint8_t>(current_timestamp >> 16);
    uuid.data[4] = static_cast<std::uint8_t>(current_timestamp >> 8);
    uuid.data[5] = static_cast<std::uint8_t>(current_timestamp);

    // Fill ver (top 4 bits are 0, 1, 1, 1)
    uuid.data[6] = (uuid.data[6] & 0x0F) | 0x70;

    // Fill var ( top 2 bits are 1, 0)
    uuid.data[8] = (uuid.data[8] & 0x3F) | 0x80;

    return uuid;
  }

 private:
  void GenerateRandomBlock(utils::span<std::uint8_t> block) {
    int i = 0;
    std::uint64_t random_value = random_generator_();

    for (auto it = block.begin(), end = block.end(); it != end; ++it, ++i) {
      if (i == sizeof(std::uint64_t)) {
        random_value = random_generator_();
        i = 0;
      }

      *it = static_cast<std::uint8_t>((random_value >> (i * 8)) & 0xFF);
    }
  }

  static std::uint64_t CurrentUnixTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               utils::datetime::WallCoarseClock::now().time_since_epoch())
        .count();
  }

 private:
  boost::random::variate_generator<utils::RandomBase*,
                                   boost::uniform_int<std::uint64_t>>
      random_generator_;

  std::uint32_t sequence_counter_{0};
  std::uint64_t previous_timestamp_{0};

  static constexpr std::uint32_t kMaxSequenceCounterValue = 0x3FFFFF;
};

compiler::ThreadLocal local_uuid_v7_generator = [] {
  return utils::WithDefaultRandom(
      [](utils::RandomBase& rng) { return UuidV7Generator(rng); });
};

}  // namespace

boost::uuids::uuid utils::generators::GenerateBoostUuidV7() {
  auto generator = local_uuid_v7_generator.Use();
  return (*generator)();
}

USERVER_NAMESPACE_END
