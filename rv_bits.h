#pragma once
#include <cstdint>
#include <tuple>

template<std::size_t l, std::size_t s, typename T = std::uint32_t>
struct rv_bitfield
{
    using value_type = T;

    static constexpr value_type val = l << s;
    static constexpr value_type shamt = s;

    constexpr operator value_type() const { return val; }
};


// & operator
template<std::size_t l, std::size_t s, typename T = std::uint32_t>
constexpr uint32_t operator &(uint32_t val, const rv_bitfield<l,s,T>& bitf)
{
    return val & bitf.val;
}

// | operator
template<std::size_t l, std::size_t s, typename T = std::uint32_t>
constexpr T operator |(uint32_t val, const rv_bitfield<l,s,T>& bitf)
{
    return val | bitf.val;
}

template<std::size_t l, std::size_t s, typename T = std::uint32_t>
constexpr T operator <<(uint32_t val, const rv_bitfield<l,s,T>& bitf)
{
    return val << bitf.shamt;
}

template<std::size_t l, std::size_t s, typename T = std::uint32_t>
constexpr T operator >>(uint32_t val, const rv_bitfield<l,s,T>& bitf)
{
    return val >> bitf.shamt;
}
