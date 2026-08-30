#include "save_data.h"

#include "level.h"
#include "progress.h"

namespace save_data
{
namespace
{

std::uint8_t checksum(const std::uint8_t* data, int length)
{
    std::uint8_t sum = 0;

    for(int i = 0; i < length; ++i)
    {
        sum = std::uint8_t(sum + data[i]);
    }

    return sum;
}

void read_legacy_costs(const std::uint8_t* in, uint16_t costs[level::COUNT])
{
    for(int i = 0; i < level::COUNT; ++i)
    {
        costs[i] = 0;
    }

    constexpr int legacy_levels = 3;

    for(int i = 0; i < legacy_levels && i < level::COUNT; ++i)
    {
        const int base = 9 + i * 2;
        costs[i] = uint16_t(in[base] | (uint16_t(in[base + 1]) << 8));
    }
}

bool decode_v1(progress::Progress& progress, const std::uint8_t in[SIZE])
{
    if(in[8] != checksum(in, 8))
    {
        return false;
    }

    progress.apply_save(in[5], in[6], in[7]);
    return true;
}

bool decode_v2(progress::Progress& progress, const std::uint8_t in[SIZE])
{
    if(in[15] != checksum(in, 15))
    {
        return false;
    }

    uint16_t costs[level::COUNT];
    read_legacy_costs(in, costs);
    progress.apply_save_v2(in[5], in[6], in[7], in[8], costs);
    return true;
}

bool decode_v3(progress::Progress& progress, const std::uint8_t in[SIZE])
{
    if(in[19] != checksum(in, 19))
    {
        return false;
    }

    uint16_t costs[level::COUNT];
    read_legacy_costs(in, costs);
    progress.apply_save_v3(in[5], in[6], in[7], in[8], costs, in[15]);
    return true;
}

} // namespace

bool encode(const progress::Progress& progress, std::uint8_t out[SIZE])
{
    out[0] = std::uint8_t(MAGIC & 0xFF);
    out[1] = std::uint8_t((MAGIC >> 8) & 0xFF);
    out[2] = std::uint8_t((MAGIC >> 16) & 0xFF);
    out[3] = std::uint8_t((MAGIC >> 24) & 0xFF);
    out[4] = VERSION;
    out[5] = std::uint8_t(progress.max_unlocked_level());
    out[6] = progress.cleared_levels_mask();
    out[7] = progress.unlocked_engines_mask();

    const uint16_t packed = progress.packed_best_stars();
    out[8] = std::uint8_t(packed & 0xFF);
    out[9] = std::uint8_t((packed >> 8) & 0xFF);

    const uint16_t* costs = progress.best_costs();

    for(int i = 0; i < level::COUNT; ++i)
    {
        const int base = 10 + i * 2;
        out[base] = std::uint8_t(costs[i] & 0xFF);
        out[base + 1] = std::uint8_t((costs[i] >> 8) & 0xFF);
    }

    out[26] = progress.intro_seen_mask();
    out[27] = 0;
    out[28] = 0;
    out[29] = 0;
    out[30] = 0;
    out[31] = checksum(out, 31);
    return true;
}

bool decode(progress::Progress& progress, const std::uint8_t in[SIZE])
{
    const std::uint32_t magic = std::uint32_t(in[0]) | (std::uint32_t(in[1]) << 8) |
                                (std::uint32_t(in[2]) << 16) | (std::uint32_t(in[3]) << 24);

    if(magic != MAGIC)
    {
        return false;
    }

    if(in[4] == VERSION_V1)
    {
        return decode_v1(progress, in);
    }

    if(in[4] == VERSION_V2)
    {
        return decode_v2(progress, in);
    }

    if(in[4] == VERSION_V3)
    {
        return decode_v3(progress, in);
    }

    if(in[4] != VERSION)
    {
        return false;
    }

    if(in[31] != checksum(in, 31))
    {
        return false;
    }

    uint16_t costs[level::COUNT];

    for(int i = 0; i < level::COUNT; ++i)
    {
        const int base = 10 + i * 2;
        costs[i] = uint16_t(in[base] | (uint16_t(in[base + 1]) << 8));
    }

    const uint16_t packed = uint16_t(in[8] | (uint16_t(in[9]) << 8));
    progress.apply_save_v4(in[5], in[6], in[7], packed, costs, in[26]);
    return true;
}

} // namespace save_data
