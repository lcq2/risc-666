#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <type_traits>
#include "rv_global.h"
#include "rv_exceptions.h"

class rv_memory
{
public:
    rv_memory() = delete;
    rv_memory(rv_uint ram_size);
    ~rv_memory();

    void map_region(rv_uint address, const uint8_t* data, size_t len, int prot);

    rv_uint fault_address() const { return fault_address_; }
    rv_exception last_exception() const { return last_exception_; }


    template<typename T> bool read(rv_uint address, T& value) const
    {
        if (address <= (ram_end_ - sizeof(T))) {
            value = *(T *)(ram_ + address);
            return true;
        }
        fault_address_ = address;
        last_exception_ = rv_exception::load_access_fault;
        return false;
    }

    template<typename T> bool write(rv_uint address, T value)
    {
        if (address <= (ram_end_ - sizeof(T))) {
            *(T *)(ram_ + address) = value;
            return true;
        }
        fault_address_ = address;
        last_exception_ = rv_exception::store_access_fault;
        return false;
    }

    bool set_brk(rv_uint offset);
    rv_uint brk() const { return brk_; }

    constexpr rv_uint stack_size() const { return (rv_uint)4_MiB; }

    rv_uint stack_begin() const { return stack_begin_; }
    void set_stack(rv_uint stack_begin);
    rv_uint stack_pointer() const { return stack_pointer_; }
    rv_uint stack_end() const { return stack_end_; }

    rv_uint ram_begin() const { return ram_begin_; }
    rv_uint ram_end() const { return ram_end_; }
    uint8_t* ram_ptr(rv_uint offset) { return ram_+offset; }
    rv_uint target_ptr(uint8_t *_ram_ptr) const { return (rv_uint)(_ram_ptr - ram_); }

    void prepare_environment(int argc, char *argv[], int optind);
private:
    uint8_t *ram_;
    rv_uint ram_begin_;
    rv_uint ram_end_;
    rv_uint stack_begin_;
    rv_uint stack_end_;
    rv_uint stack_pointer_;
    rv_uint brk_;

    mutable rv_uint fault_address_;
    mutable rv_exception last_exception_;
};
