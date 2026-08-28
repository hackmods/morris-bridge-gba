#ifndef MORRIS_BRIDGE_SAVE_DATA_H
#define MORRIS_BRIDGE_SAVE_DATA_H

/**
 * @file
 * Platform-neutral progress serialization for SRAM / emulator saves.
 */

#include <cstdint>

namespace progress
{
class Progress;
}

namespace save_data
{

inline constexpr int SIZE = 9;
inline constexpr std::uint32_t MAGIC = 0x47425254; // 'TRBG' little-endian
inline constexpr std::uint8_t VERSION = 1;

[[nodiscard]] bool encode(const progress::Progress& progress, std::uint8_t out[SIZE]);

[[nodiscard]] bool decode(progress::Progress& progress, const std::uint8_t in[SIZE]);

} // namespace save_data

#endif
