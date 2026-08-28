#include "physics.h"

#include <stdint.h>

namespace physics
{

int isqrt64(int64_t value)
{
    if(value <= 0)
    {
        return 0;
    }

    // Restore one bit-pair at a time. `one` starts at the highest even bit
    // that can contribute to a 32-bit root of a 64-bit radicand.
    uint64_t op = static_cast<uint64_t>(value);
    uint64_t res = 0;
    uint64_t one = uint64_t(1) << 62;

    while(one > op)
    {
        one >>= 2;
    }

    while(one != 0)
    {
        if(op >= res + one)
        {
            op -= res + one;
            res = (res >> 1) + one;
        }
        else
        {
            res >>= 1;
        }

        one >>= 2;
    }

    return static_cast<int>(res);
}

bn::fixed approx_sqrt(bn::fixed value)
{
    if(value <= 0)
    {
        return 0;
    }

    // x is 20.12. We want sqrt(x) still in 20.12:
    //   result.data() = sqrt(x.data() << 12)
    const int64_t shifted = static_cast<int64_t>(value.data()) << bn::fixed::precision();
    return bn::fixed::from_data(isqrt64(shifted));
}

bn::fixed squared_distance(bn::fixed_point a, bn::fixed_point b)
{
    const bn::fixed dx = a.x() - b.x();
    const bn::fixed dy = a.y() - b.y();

    // Pixel deltas fit in 8 bits; safe 64-bit multiply avoids the 20.12
    // half-precision shortcut dropping low bits on large spans.
    return dx.safe_multiplication(dx) + dy.safe_multiplication(dy);
}

bn::fixed distance(bn::fixed_point a, bn::fixed_point b)
{
    return approx_sqrt(squared_distance(a, b));
}

bn::fixed_point lerp(bn::fixed_point a, bn::fixed_point b, bn::fixed t)
{
    const bn::fixed_point delta = b - a;
    return bn::fixed_point(a.x() + delta.x().safe_multiplication(t),
                           a.y() + delta.y().safe_multiplication(t));
}

bn::fixed tension(bn::fixed base_weight, bn::fixed distance_from_train, bn::fixed strut_length)
{
    if(strut_length <= 0)
    {
        return 0;
    }

    if(distance_from_train < 0)
    {
        distance_from_train = 0;
    }

    // Tension = BaseWeight × (DistanceFromTrain / StrutLength)
    const bn::fixed ratio = distance_from_train.safe_division(strut_length);
    return base_weight.safe_multiplication(ratio);
}

bn::fixed max_tension_for_length(bn::fixed length)
{
    if(length <= 0)
    {
        return BASE_MAX_TENSION;
    }

    // Inverse-length rating: a 48px strut gets BASE_MAX_TENSION (8);
    // a 96px strut gets 4, which equals BASE_WEIGHT at the far end.
    bn::fixed rated = BASE_MAX_TENSION.safe_multiplication(SAFE_LENGTH).safe_division(length);

    if(rated < 1)
    {
        rated = 1;
    }

    return rated;
}

} // namespace physics
