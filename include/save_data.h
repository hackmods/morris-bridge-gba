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

inline constexpr int SIZE_V1 = 9;
inline constexpr int SIZE_V2 = 16;
inline constexpr int SIZE_V3 = 20;
inline constexpr int SIZE = 32;
inline constexpr std::uint32_t MAGIC = 0x47425254; // 'TRBG' little-endian
inline constexpr std::uint8_t VERSION_V1 = 1;
inline constexpr std::uint8_t VERSION_V2 = 2;
inline constexpr std::uint8_t VERSION_V3 = 3;
inline constexpr std::uint8_t VERSION = 4;

[[nodiscard]] bool encode(const progress::Progress& progress, std::uint8_t out[SIZE]);

[[nodiscard]] bool decode(progress::Progress& progress, const std::uint8_t in[SIZE]);

} // namespace save_data

#endif
