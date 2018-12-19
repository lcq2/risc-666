#pragma once
#include <cstdint>

template<std::size_t l, std::size_t s, typename T = std::uint32_t>
struct rv_bitfield
{
    using value_type = T;

    static constexpr value_type mask = l << s;
    static constexpr value_type shamt = s;

    constexpr operator value_type() const { return mask; }
};

// & operator
template<std::size_t l, std::size_t s, typename T = std::uint32_t>
constexpr T operator &(T val, const rv_bitfield<l,s,T>& bitf)
{
    return val & bitf.mask;
}

// | operator
template<std::size_t l, std::size_t s, typename T = std::uint32_t>
constexpr T operator |(T val, const rv_bitfield<l,s,T>& bitf)
{
    return val | bitf.mask;
}

template<std::size_t l, std::size_t s, typename T = std::uint32_t>
constexpr T operator <<(T val, const rv_bitfield<l,s,T>& bitf)
{
    return val << bitf.shamt;
}

template<std::size_t l, std::size_t s, typename T = std::uint32_t>
constexpr T operator >>(T val, const rv_bitfield<l,s,T>& bitf)
{
    return val >> bitf.shamt;
}
