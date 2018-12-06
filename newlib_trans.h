#pragma once
#include <cstdint>

struct newlib_timeval
{
    uint32_t tv_sec;
    uint32_t tv_usec;
};

struct newlib_timespec
{
    uint32_t tv_sec;
    uint32_t tv_nsec;
};

struct  newlib_stat
{
    unsigned long long st_dev;
    unsigned long long st_ino;
    unsigned int st_mode;
    unsigned int st_nlink;
    unsigned int st_uid;
    unsigned int st_gid;
    unsigned long long st_rdev;
    unsigned long long __pad1;
    long long st_size;
    int st_blksize;
    int __pad2;
    long long st_blocks;
    struct newlib_timespec st_atim;
    struct newlib_timespec st_mtim;
    struct newlib_timespec st_ctim;
    int __glibc_reserved[2];
};

int newlib_translate_open_flags(int newlib_flags);
void newlib_translate_stat(newlib_stat *dst, struct stat *src);
void newlib_translate_timeval(newlib_timeval *dst, struct timeval *src);