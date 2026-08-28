#ifndef BN_VECTOR_H
#define BN_VECTOR_H

#include <cstddef>
#include <utility>

namespace bn
{

template<typename Type, int MaxSize>
class vector
{
public:
    using iterator = Type*;
    using const_iterator = const Type*;

    constexpr vector() = default;

    constexpr vector(const vector& other)
    {
        *this = other;
    }

    constexpr vector& operator=(const vector& other)
    {
        if(this != &other)
        {
            _size = other._size;

            for(int i = 0; i < _size; ++i)
            {
                _data[i] = other._data[i];
            }
        }

        return *this;
    }

    [[nodiscard]] constexpr int size() const
    {
        return _size;
    }

    [[nodiscard]] constexpr int max_size() const
    {
        return MaxSize;
    }

    [[nodiscard]] constexpr bool empty() const
    {
        return _size == 0;
    }

    [[nodiscard]] constexpr bool full() const
    {
        return _size == MaxSize;
    }

    [[nodiscard]] constexpr Type& operator[](int index)
    {
        return _data[index];
    }

    [[nodiscard]] constexpr const Type& operator[](int index) const
    {
        return _data[index];
    }

    [[nodiscard]] constexpr iterator begin()
    {
        return _data;
    }

    [[nodiscard]] constexpr const_iterator begin() const
    {
        return _data;
    }

    [[nodiscard]] constexpr iterator end()
    {
        return _data + _size;
    }

    [[nodiscard]] constexpr const_iterator end() const
    {
        return _data + _size;
    }

    constexpr void push_back(const Type& value)
    {
        _data[_size] = value;
        ++_size;
    }

    constexpr void pop_back()
    {
        --_size;
    }

    constexpr void clear()
    {
        _size = 0;
    }

    constexpr iterator erase(iterator position)
    {
        const int index = int(position - _data);

        for(int i = index; i < _size - 1; ++i)
        {
            _data[i] = _data[i + 1];
        }

        --_size;
        return _data + index;
    }

private:
    Type _data[MaxSize]{};
    int _size = 0;
};

} // namespace bn

#endif
