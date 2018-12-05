#pragma once
#include <cstdint>

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

// native integer type (not C integer)
// we only support XLEN == 32, so this is always a 32bit integer (signed/unsigned)
using rv_uint = uint32_t;
using rv_int = int32_t;

// native long integer type (not C long)
// 64bit for XLEN == 32, 128bit for XLEN == 64
using rv_ulong = uint64_t;
using rv_long = int64_t;

constexpr unsigned long long operator ""_MiB(unsigned long long v)
{
    return v*1024*1024;
}