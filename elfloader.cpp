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

#define ELF_RISCV_MACHINE 243

template<typename T>
auto make_ptr(const void *data, size_t offset = 0)
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
    auto *elf_hdr = make_ptr<Elf32_Ehdr>(buf);
    if (!check_magic(elf_hdr)) {
        throw std::runtime_error("invalid ELF file");
    }

    // store ELF header for later use
    memcpy(&header_, elf_hdr, sizeof(Elf32_Ehdr));

    // ensure this is a 32bit executable
    if (elf_hdr->e_ident[EI_CLASS] != ELFCLASS32) {
        throw std::runtime_error("only 32bit ELFs supported");
    }

    // ensure RISC-V machine
    if (elf_hdr->e_machine != ELF_RISCV_MACHINE) {
        throw std::runtime_error("invalid e_machine, only RISC-V supported");
    }

    // store program headers into segments_ array for easy access
    if (elf_hdr->e_phnum == 0) {
        throw std::runtime_error("invalid ELF, at least one program header required");
    }

    fprintf(stderr, "[i] loading program segments...\n");
    auto *phdr = make_ptr<Elf32_Phdr>(elf_hdr, elf_hdr->e_phoff);
    for (size_t i = 0; i < elf_hdr->e_phnum; ++i) {
        if (phdr->p_type == PT_LOAD) {
            fprintf(stderr, "[i] segment %d - vaddr: 0x%08x, vsize: %d\n", i, phdr->p_vaddr, phdr->p_memsz);
            segments_.emplace_back(*phdr);
        }
        phdr = make_ptr<Elf32_Phdr>(phdr, elf_hdr->e_phentsize);
    }

    entry_point_ = elf_hdr->e_entry;

    fprintf(stderr, "[i] entry point at 0x%08x\n", entry_point_);
}

Elf32_Addr elf_loader::segment_vaddress(size_t index) const
{
    assert(index < segments_.size());
    return segments_[index].p_vaddr;
}

Elf32_Word elf_loader::segment_vsize(size_t index) const
{
    assert(index < segments_.size());
    return segments_[index].p_memsz;
}

Elf32_Word elf_loader::segment_psize(size_t index) const
{
    assert(index < segments_.size());
    return segments_[index].p_filesz;
}

Elf32_Word elf_loader::segment_flags(size_t index) const
{
    assert(index < segments_.size());
    return segments_[index].p_flags;
}

int elf_loader::segment_protection(size_t index) const
{
    assert(index < segments_.size());
    const auto& seg = segments_[index];

    int prot = PROT_NONE;
    if ((seg.p_flags & PF_W) == PF_W)
        prot |= PROT_WRITE;
    if ((seg.p_flags & PF_R) == PF_R)
        prot |= PROT_READ;
    if ((seg.p_flags & PF_X) == PF_X)
        prot |= PROT_EXEC;
    return prot;
}

const uint8_t* elf_loader::segment_data(size_t index) const
{
    assert(index < segments_.size());
    return make_ptr<uint8_t>(buffer_.data(), segments_[index].p_offset);
}