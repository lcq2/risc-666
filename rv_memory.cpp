#include <cstring>
#include <cassert>
#include "rv_exceptions.h"
#include "rv_memory.h"

constexpr rv_uint RV_STACK_SIZE = 4*1024*1024;

rv_memory::rv_memory(rv_uint ram_size)
{
    ram_ = new uint8_t[ram_size]();
    ram_begin_ = 0;
    ram_end_ = ram_size;
    stack_begin_ = ram_end_-sizeof(rv_uint);
    stack_end_ = stack_begin_ - RV_STACK_SIZE;
}

rv_memory::~rv_memory()
{
    if (ram_ != nullptr) {
        delete [] ram_;
    }
}

void rv_memory::map_region(rv_uint address, const uint8_t *data, size_t len, int prot)
{
    assert(address < (ram_end_-len));
    memcpy(ram_+address, data, len);
}

bool rv_memory::set_brk(rv_uint offset)
{
    if (offset > (ram_end_ - RV_STACK_SIZE))
        return false;
    brk_ = offset;
    return true;
}

void rv_memory::prepare_environment(int argc, char *argv[], int optind)
{
    // optind points to the first non option argument, which is the target to emulate
    // after the executable name, we have the arguments to pass to the emulated target
    int num_args = argc - optind;

    // +1 for the null entry
    rv_uint target_stack = stack_begin()-(num_args+1)*sizeof(rv_uint);
    stack_pointer_ = target_stack;

    write(target_stack, num_args);
    target_stack += sizeof(rv_uint);

    // argument strings are stored at 0x100
    // we cannot use 0x0 because according to RISC-V specs, reading from 0 is hardwired to fault
    // +1 for the null entry
    char *target_env = reinterpret_cast<char*>(ram_ptr(0x100));

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