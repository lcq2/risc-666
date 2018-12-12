#include "elfloader.h"
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <fmt/format.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <functional>

#define ELF_RISCV_MACHINE 243

template<typename T>
auto offset_ptr(const void *data, size_t offset = 0)
{
    return reinterpret_cast<const T*>(reinterpret_cast<const uint8_t*>(data)+offset);
}

bool elf_loader::check_magic(const Elf32_Ehdr *hdr) const
{
    return hdr->e_ident[EI_MAG0] == ELFMAG0 &&
        hdr->e_ident[EI_MAG1] == ELFMAG1 &&
        hdr->e_ident[EI_MAG2] == ELFMAG2 &&
        hdr->e_ident[EI_MAG3] == ELFMAG3;
}

elf_loader::~elf_loader()
{

}

void elf_loader::load()
{
    // load target ELF into memory
    struct stat st;
    if (stat(filename_.c_str(), &st) == -1) {
        throw std::runtime_error("stat() failed");
    }

    buffer_.resize(st.st_size);

    int fd = open(filename_.c_str(), O_RDONLY);
    if (fd == -1) {
        throw std::runtime_error("open() failed");
    }

    ssize_t bytes_read = read(fd, buffer_.data(), st.st_size);
    close(fd);
    if (bytes_read == -1 || bytes_read != st.st_size) {
        throw std::runtime_error("read() failed");
    }

    const uint8_t*const buf = buffer_.data();

    fprintf(stderr, "[i] checking ELF file...\n");
    // check elf magic
    header_ = offset_ptr<Elf32_Ehdr>(buf);
    if (!check_magic(header_)) {
        throw std::runtime_error("invalid ELF file");
    }

    // ensure this is a 32bit executable
    if (header_->e_ident[EI_CLASS] != ELFCLASS32) {
        throw std::runtime_error("only 32bit ELFs supported");
    }

    // ensure RISC-V machine
    if (header_->e_machine != ELF_RISCV_MACHINE) {
        throw std::runtime_error("invalid e_machine, only RISC-V supported");
    }

    // store program headers into segments_ array for easy access
    if (header_->e_phnum == 0) {
        throw std::runtime_error("invalid ELF, at least one program header required");
    }

    fprintf(stderr, "[i] loading program segments...\n");
    auto *phdr = offset_ptr<Elf32_Phdr>(header_, header_->e_phoff);
    for (size_t i = 0; i < header_->e_phnum; ++i) {
        // we care only about PT_LOAD, since these segments are actually mapped
        // for now, we ignore the protection flag
        if (phdr->p_type == PT_LOAD) {
            fprintf(stderr, "[i] segment %d - vaddr: 0x%08x, vsize: %d\n", i, phdr->p_vaddr, phdr->p_memsz);
            segments_.emplace_back(phdr);
        }
        phdr = offset_ptr<Elf32_Phdr>(phdr, header_->e_phentsize);
    }

    fprintf(stderr, "[i] loading symbols...\n");
    // first we need to locate the symbol table within the sections
    auto *shdr = offset_ptr<Elf32_Shdr>(header_, header_->e_shoff);
    for (size_t i = 0; i < header_->e_shnum; ++i) {
        if (shdr->sh_type == SHT_SYMTAB) {
            symbol_table_ = shdr;
        }
        sections_.push_back(shdr);
        shdr = offset_ptr<Elf32_Shdr>(shdr, header_->e_shentsize);
    }

    if (sections_.empty()) {
        throw std::runtime_error("missing sections");
    }
    if (symbol_table_ == nullptr) {
        throw std::runtime_error("missing symbol table");
    }

    uint32_t num_syms = symbol_table_->sh_size / symbol_table_->sh_entsize;
    fprintf(stderr, "[i] found %d symbols...\n", num_syms);

    // scan all the symbol table and build a "name" => "address" map
    auto *psym = offset_ptr<Elf32_Sym>(header_, symbol_table_->sh_offset);
    auto strtable = offset_ptr<const char>(header_, sections_[symbol_table_->sh_link]->sh_offset);
    for (uint32_t i = 0; i < num_syms; ++i) {
        // first of all, we're interested only in GLOBAL symbols of type STT_FUNC
        // note: st_name IS NOT a pointer to a string, but an index into the string table
        if (psym->st_info == ELF32_ST_INFO(STB_GLOBAL, STT_FUNC)) {
            // now lookup name
            auto sym_name = offset_ptr<const char>(strtable, psym->st_name);
            symbols_.emplace(sym_name, psym->st_value);
        }
        psym = offset_ptr<Elf32_Sym>(psym, symbol_table_->sh_entsize);
    }

    fprintf(stderr, "[i] entry point at 0x%08x\n", entry_point());
}