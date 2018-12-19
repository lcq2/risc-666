#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cassert>
#include <unordered_map>
#include <tuple>
#include <elf.h>

class elf_segment
{
public:
    elf_segment() = default;
    elf_segment(const Elf32_Phdr* s) : segment_{s} {}

    auto offset() const { return segment_->p_offset; }
    auto virtual_address() const { return segment_->p_vaddr; }
    auto physical_address() const { return segment_->p_paddr; }
    auto file_size() const { return segment_->p_filesz; }
    auto memory_size() const { return segment_->p_memsz; }
    auto alignment() const { return segment_->p_align; }
    uint8_t protection() const;

private:
    const Elf32_Phdr* segment_ = nullptr;
};

class elf_loader
{
public:
    explicit elf_loader(std::string filename) : filename_{std::move(filename)} {}
    ~elf_loader();
    void load();

    const auto& segments() const { return segments_; }

    template<typename T>
    const T* pointer_to(const elf_segment& segm) const
    {
        return reinterpret_cast<const T*>(buffer_.data() + segm.offset());
    }

    Elf32_Addr entry_point() const { return header_->e_entry; }

private:
    bool check_magic(const Elf32_Ehdr* hdr) const;

private:
    std::string filename_;
    std::vector<uint8_t> buffer_;
    std::vector<elf_segment> segments_;
    std::vector<const Elf32_Shdr*> sections_;
    const Elf32_Shdr* symbol_table_ = nullptr;

    // this definitely takes too much memory, needs to be fixed in the future
    std::unordered_map<std::string, Elf32_Addr> symbols_;
    const Elf32_Ehdr *header_ = nullptr;
};
