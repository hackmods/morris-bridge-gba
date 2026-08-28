#ifndef BN_OPTIONAL_H
#define BN_OPTIONAL_H

namespace bn
{

template<typename Type>
class optional
{
public:
    constexpr optional() = default;

    constexpr optional(const Type& value) :
        _value(value),
        _has(true)
    {
    }

    constexpr optional& operator=(const Type& value)
    {
        _value = value;
        _has = true;
        return *this;
    }

    [[nodiscard]] constexpr bool has_value() const
    {
        return _has;
    }

    [[nodiscard]] constexpr explicit operator bool() const
    {
        return _has;
    }

    [[nodiscard]] constexpr Type& value()
    {
        return _value;
    }

    [[nodiscard]] constexpr const Type& value() const
    {
        return _value;
    }

    [[nodiscard]] constexpr Type& operator*()
    {
        return _value;
    }

    [[nodiscard]] constexpr const Type& operator*() const
    {
        return _value;
    }

    constexpr void reset()
    {
        _has = false;
    }

private:
    Type _value{};
    bool _has = false;
};

} // namespace bn

#endif
