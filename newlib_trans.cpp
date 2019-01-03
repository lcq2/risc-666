#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "newlib_trans.h"

int newlib_translate_open_flags(int newlib_flags)
{
#define	NEWLIB__FOPEN		(-1)	/* from sys/file.h, kernel use only */
#define	NEWLIB__FREAD		0x0001	/* read enabled */
#define	NEWLIB__FWRITE		0x0002	/* write enabled */
#define	NEWLIB__FAPPEND	0x0008	/* append (writes guaranteed at the end) */
#define	NEWLIB__FMARK		0x0010	/* internal; mark during gc() */
#define	NEWLIB__FDEFER		0x0020	/* internal; defer for next gc pass */
#define	NEWLIB__FASYNC		0x0040	/* signal pgrp when data ready */
#define	NEWLIB__FSHLOCK	0x0080	/* BSD flock() shared lock present */
#define	NEWLIB__FEXLOCK	0x0100	/* BSD flock() exclusive lock present */
#define	NEWLIB__FCREAT		0x0200	/* open with file create */
#define	NEWLIB__FTRUNC		0x0400	/* open with truncation */
#define	NEWLIB__FEXCL		0x0800	/* error on open if file exists */
#define	NEWLIB__FNBIO		0x1000	/* non blocking I/O (sys5 style) */
#define	NEWLIB__FSYNC		0x2000	/* do all writes synchronously */
#define	NEWLIB__FNONBLOCK	0x4000	/* non blocking I/O (POSIX style) */
#define	NEWLIB__FNDELAY	_FNONBLOCK	/* non blocking I/O (4.2 style) */
#define	NEWLIB__FNOCTTY	0x8000	/* don't assign a ctty on this open */
#define	NEWLIB_O_ACCMODE	(NEWLIB_O_RDONLY|NEWLIB_O_WRONLY|NEWLIB_O_RDWR)
#define	NEWLIB_O_RDONLY	0		/* +1 == FREAD */
#define	NEWLIB_O_WRONLY	1		/* +1 == FWRITE */
#define	NEWLIB_O_RDWR		2		/* +1 == FREAD|FWRITE */
#define	NEWLIB_O_APPEND	NEWLIB__FAPPEND
#define	NEWLIB_O_CREAT		NEWLIB__FCREAT
#define	NEWLIB_O_TRUNC		NEWLIB__FTRUNC
#define	NEWLIB_O_EXCL		NEWLIB__FEXCL
#define NEWLIB_O_SYNC		NEWLIB__FSYNC
#define	NEWLIB_O_NONBLOCK	NEWLIB__FNONBLOCK
#define	NEWLIB_O_NOCTTY	NEWLIB__FNOCTTY

    int flags = 0;
    if (newlib_flags & NEWLIB_O_RDONLY)
        flags |= O_RDONLY;
    if (newlib_flags & NEWLIB_O_WRONLY)
        flags |= O_WRONLY;
    if (newlib_flags & NEWLIB_O_RDWR)
        flags |= O_RDWR;
    if (newlib_flags & NEWLIB_O_APPEND)
        flags |= O_APPEND;
    if (newlib_flags & NEWLIB_O_CREAT)
        flags |= O_CREAT;
    if (newlib_flags & NEWLIB_O_TRUNC)
        flags |= O_TRUNC;
    if (newlib_flags & NEWLIB_O_EXCL)
        flags |= O_EXCL;
    if (newlib_flags & NEWLIB_O_SYNC)
        flags |= O_SYNC;
    if (newlib_flags & NEWLIB_O_NONBLOCK)
        flags |= O_NONBLOCK;
    if (newlib_flags & NEWLIB_O_NOCTTY)
        flags |= O_NOCTTY;
    return flags;
}

void newlib_translate_stat(newlib_stat *dst, struct stat *src)
{
    dst->st_dev = src->st_dev;
    dst->st_ino = src->st_ino;
    dst->st_mode = src->st_mode;
    dst->st_nlink = (unsigned int)src->st_nlink;
    dst->st_uid = src->st_uid;
    dst->st_gid = src->st_gid;
    dst->st_rdev = src->st_rdev;
    dst->st_size = src->st_size;
    dst->st_blksize = (int)src->st_blksize;
    dst->st_blocks = src->st_blocks;
#ifdef RISC_666_LINUX
    dst->st_atim.tv_sec = (uint32_t)src->st_atim.tv_sec;
    dst->st_atim.tv_nsec = (uint32_t)src->st_atim.tv_nsec;
    dst->st_mtim.tv_sec = (uint32_t)src->st_mtim.tv_sec;
    dst->st_mtim.tv_nsec = (uint32_t)src->st_mtim.tv_nsec;
    dst->st_ctim.tv_sec = (uint32_t)src->st_ctim.tv_sec;
    dst->st_ctim.tv_nsec = (uint32_t)src->st_ctim.tv_nsec;
#elif RISC_666_OSX
    dst->st_atim.tv_sec = (uint32_t)src->st_atimespec.tv_sec;
    dst->st_atim.tv_nsec = (uint32_t)src->st_atimespec.tv_nsec;
    dst->st_mtim.tv_sec = (uint32_t)src->st_mtimespec.tv_sec;
    dst->st_mtim.tv_nsec = (uint32_t)src->st_mtimespec.tv_nsec;
    dst->st_ctim.tv_sec = (uint32_t)src->st_ctimespec.tv_sec;
    dst->st_ctim.tv_nsec = (uint32_t)src->st_ctimespec.tv_nsec;
#endif
}

void newlib_translate_timeval(newlib_timeval *dst, struct timeval *src)
{
    dst->tv_sec = (uint32_t)src->tv_sec;
    dst->tv_usec = (uint32_t)src->tv_usec;
}