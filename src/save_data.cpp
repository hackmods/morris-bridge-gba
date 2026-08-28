#include "save_data.h"

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
    out[8] = checksum(out, 8);
    return true;
}

bool decode(progress::Progress& progress, const std::uint8_t in[SIZE])
{
    const std::uint32_t magic = std::uint32_t(in[0]) | (std::uint32_t(in[1]) << 8) |
                                (std::uint32_t(in[2]) << 16) | (std::uint32_t(in[3]) << 24);

    if(magic != MAGIC || in[4] != VERSION)
    {
        return false;
    }

    if(in[8] != checksum(in, 8))
    {
        return false;
    }

    progress.apply_save(in[5], in[6], in[7]);
    return true;
}

} // namespace save_data
