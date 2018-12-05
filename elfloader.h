//
// Created by quake2 on 12/2/18.
//
#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cassert>
#include <elf.h>

class elf_loader
{
public:
    explicit elf_loader(std::string filename) : filename_{std::move(filename)} {}
    ~elf_loader();
    void load();

    Elf32_Addr segment_vaddress(size_t index) const;
    Elf32_Word segment_vsize(size_t index) const;
    Elf32_Word segment_psize(size_t index) const;
    Elf32_Word segment_flags(size_t index) const;
    const uint8_t* segment_data(size_t index) const;
    int segment_protection(size_t index) const;

    size_t num_segments() const { return segments_.size(); }
    Elf32_Addr entry_point() const { return entry_point_; }

private:
    bool check_magic(const Elf32_Ehdr* hdr) const;

private:
    std::string filename_;
    std::vector<uint8_t> buffer_;
    std::vector<Elf32_Phdr> segments_;
    Elf32_Ehdr header_;
    Elf32_Addr entry_point_;
};
