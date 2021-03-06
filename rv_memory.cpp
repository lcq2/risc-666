#include <cstring>
#include <cassert>
#include <stdexcept>
#include "rv_exceptions.h"
#include "rv_memory.h"

constexpr rv_uint RV_STACK_SIZE = 4*1024*1024;

rv_memory::rv_memory(rv_uint ram_size)
{
    ram_ = new uint8_t[ram_size]();
    ram_begin_ = 0;
    ram_end_ = ram_size;

    // reserve ram_size/4096 "entries" in our mpu
    mpu_.resize(ram_size >> 12);
    std::fill(mpu_.begin(), mpu_.end(), 0);
}

rv_memory::~rv_memory()
{
    delete [] ram_;
}

void rv_memory::set_region(rv_uint address, const uint8_t *data, size_t len)
{
    assert(address <= (ram_end_-len));
    assert(data != nullptr);

    // fill region with provided data
    memcpy(ram_+address, data, len);
}

void rv_memory::protect_region(rv_uint address, size_t len, uint8_t prot)
{
    assert(address <= (ram_end_-len));

    // setup protection flags for the requested range up to page boundary
    // this will overwrite whatever flags were set previously
    if (len % 0x1000 != 0)
        len = len + (0x1000 - len%0x1000);

    auto npages = len >> 12;
    auto pageindex = address >> 12;
    std::fill(mpu_.begin()+pageindex, mpu_.begin()+pageindex+npages+1, prot);
}

bool rv_memory::set_brk(rv_uint offset)
{
    if (offset > ram_end_ || offset < stack_begin_)
        return false;
    brk_ = offset;
    return true;
}

void rv_memory::prepare_environment(int argc, char *argv[], int optind)
{
    // optind points to the first non option argument, which is the target to emulate
    // after the executable name, we have the arguments to pass to the emulated target
    int num_args = argc - optind;

    // 1 entry for argc
    // n entries for argv[n]
    // 1 entry for the null entry
    rv_uint target_stack = stack_begin() - sizeof(rv_uint) -(num_args+1)*sizeof(rv_uint);
    stack_pointer_ = target_stack;

    write(target_stack, num_args);
    target_stack += sizeof(rv_uint);

    // argument strings are stored at 0x100
    // we cannot use 0x0 because according to RISC-V specs, reading from 0 is hardwired to fault
    char *target_env = reinterpret_cast<char*>(ram_ptr(0x100));

    // fix this mess...
    while (optind < argc) {
        auto arg_len = strlen(argv[optind]);
        if (arg_len > 31)
            arg_len = 31;
        strncpy(target_env, argv[optind], arg_len);
        target_env[arg_len] = '\0';
        write(target_stack, target_ptr((uint8_t *) target_env));
        target_stack += sizeof(rv_uint);
        target_env += arg_len+1;
        optind += 1;
    }
    write(target_stack, 0x00);
}

void rv_memory::set_stack(rv_uint stack_begin)
{
    if (stack_begin > ram_end_) {
        throw std::runtime_error("invalid stack location");
    }
    stack_begin_ = stack_begin;
    stack_end_ = stack_begin - stack_size();
}