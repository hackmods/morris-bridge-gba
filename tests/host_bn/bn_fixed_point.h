#ifndef BN_FIXED_POINT_H
#define BN_FIXED_POINT_H

#include "bn_fixed.h"

namespace bn
{

template<int Precision>
class fixed_point_t
{
public:
    constexpr fixed_point_t() = default;

    constexpr fixed_point_t(fixed_t<Precision> x, fixed_t<Precision> y) :
        _x(x),
        _y(y)
    {
    }

    [[nodiscard]] constexpr fixed_t<Precision> x() const
    {
        return _x;
    }

    constexpr void set_x(fixed_t<Precision> x)
    {
        _x = x;
    }

    [[nodiscard]] constexpr fixed_t<Precision> y() const
    {
        return _y;
    }

    constexpr void set_y(fixed_t<Precision> y)
    {
        _y = y;
    }

    [[nodiscard]] constexpr friend fixed_point_t operator+(const fixed_point_t& a, const fixed_point_t& b)
    {
        return fixed_point_t(a._x + b._x, a._y + b._y);
    }

    [[nodiscard]] constexpr friend fixed_point_t operator-(const fixed_point_t& a, const fixed_point_t& b)
    {
        return fixed_point_t(a._x - b._x, a._y - b._y);
    }

    [[nodiscard]] constexpr friend fixed_point_t operator*(const fixed_point_t& a, fixed_t<Precision> b)
    {
        return fixed_point_t(a._x * b, a._y * b);
    }

    [[nodiscard]] constexpr friend bool operator==(const fixed_point_t& a, const fixed_point_t& b) = default;

private:
    fixed_t<Precision> _x = 0;
    fixed_t<Precision> _y = 0;
};

using fixed_point = fixed_point_t<12>;

} // namespace bn

#endif
