#ifndef BN_FIXED_H
#define BN_FIXED_H

/**
 * Host-side mock of Butano's `bn::fixed` / `bn::fixed_t<12>` (20.12).
 *
 * Arithmetic matches the engine's `safe_multiplication` / `safe_division`
 * (64-bit intermediates, then divide by 4096) so the PC unit tests exercise
 * the same bit shifts the GBA ROM will use.
 */

#include <cstdint>
#include <compare>

namespace bn
{

template<int Precision>
class fixed_t
{
    static_assert(Precision > 0 && Precision < 31, "Invalid precision");

public:
    [[nodiscard]] static constexpr fixed_t from_data(int data)
    {
        fixed_t result;
        result._data = data;
        return result;
    }

    [[nodiscard]] static constexpr int precision()
    {
        return Precision;
    }

    [[nodiscard]] static constexpr int scale()
    {
        return 1 << Precision;
    }

    constexpr fixed_t() = default;

    constexpr fixed_t(int value) :
        _data(value * scale())
    {
    }

    constexpr fixed_t(unsigned value) :
        _data(int(value << Precision))
    {
    }

    [[nodiscard]] constexpr int data() const
    {
        return _data;
    }

    [[nodiscard]] constexpr int integer() const
    {
        return _data / scale();
    }

    [[nodiscard]] constexpr int shift_integer() const
    {
        return _data >> Precision;
    }

    [[nodiscard]] constexpr int right_shift_integer() const
    {
        return shift_integer();
    }

    [[nodiscard]] constexpr fixed_t safe_multiplication(int value) const
    {
        return from_data(_data * value);
    }

    [[nodiscard]] constexpr fixed_t safe_multiplication(fixed_t other) const
    {
        return from_data(int((int64_t(_data) * other._data) / scale()));
    }

    [[nodiscard]] constexpr fixed_t safe_division(int value) const
    {
        return from_data(_data / value);
    }

    [[nodiscard]] constexpr fixed_t safe_division(fixed_t other) const
    {
        return from_data(int((int64_t(_data) * scale()) / other._data));
    }

    [[nodiscard]] constexpr fixed_t multiplication(fixed_t other) const
    {
        return safe_multiplication(other);
    }

    [[nodiscard]] constexpr fixed_t division(fixed_t other) const
    {
        return safe_division(other);
    }

    [[nodiscard]] constexpr fixed_t operator-() const
    {
        return from_data(-_data);
    }

    constexpr fixed_t& operator+=(fixed_t other)
    {
        _data += other._data;
        return *this;
    }

    constexpr fixed_t& operator-=(fixed_t other)
    {
        _data -= other._data;
        return *this;
    }

    constexpr fixed_t& operator*=(fixed_t other)
    {
        *this = safe_multiplication(other);
        return *this;
    }

    constexpr fixed_t& operator/=(fixed_t other)
    {
        *this = safe_division(other);
        return *this;
    }

    [[nodiscard]] constexpr friend fixed_t operator+(fixed_t a, fixed_t b)
    {
        return from_data(a._data + b._data);
    }

    [[nodiscard]] constexpr friend fixed_t operator-(fixed_t a, fixed_t b)
    {
        return from_data(a._data - b._data);
    }

    [[nodiscard]] constexpr friend fixed_t operator*(fixed_t a, int b)
    {
        return a.safe_multiplication(b);
    }

    [[nodiscard]] constexpr friend fixed_t operator*(fixed_t a, fixed_t b)
    {
        return a.safe_multiplication(b);
    }

    [[nodiscard]] constexpr friend fixed_t operator/(fixed_t a, int b)
    {
        return a.safe_division(b);
    }

    [[nodiscard]] constexpr friend fixed_t operator/(fixed_t a, fixed_t b)
    {
        return a.safe_division(b);
    }

    [[nodiscard]] constexpr friend auto operator<=>(fixed_t a, fixed_t b) = default;

private:
    int _data = 0;
};

using fixed = fixed_t<12>;

} // namespace bn

#endif
