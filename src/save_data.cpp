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

bool decode_v1(progress::Progress& progress, const std::uint8_t in[SIZE])
{
    if(in[8] != checksum(in, 8))
    {
        return false;
    }

    progress.apply_save(in[5], in[6], in[7]);
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
    out[8] = progress.packed_best_stars();

    const uint16_t* costs = progress.best_costs();

    for(int i = 0; i < level::COUNT; ++i)
    {
        const int base = 9 + i * 2;
        out[base] = std::uint8_t(costs[i] & 0xFF);
        out[base + 1] = std::uint8_t((costs[i] >> 8) & 0xFF);
    }

    out[15] = checksum(out, 15);
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

    if(in[4] != VERSION)
    {
        return false;
    }

    if(in[15] != checksum(in, 15))
    {
        return false;
    }

    uint16_t costs[level::COUNT];

    for(int i = 0; i < level::COUNT; ++i)
    {
        const int base = 9 + i * 2;
        costs[i] = uint16_t(in[base] | (uint16_t(in[base + 1]) << 8));
    }

    progress.apply_save_v2(in[5], in[6], in[7], in[8], costs);
    return true;
}

} // namespace save_data
