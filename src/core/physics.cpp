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

} // namespace physics
