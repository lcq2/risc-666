#pragma once
#include <cstdint>
#include <vector>
#include "rv_global.h"
#include "rv_bits.h"
#include "rv_exceptions.h"

constexpr auto RV_MEMORY_R = rv_bitfield<1,0,uint8_t>{};
constexpr auto RV_MEMORY_W = rv_bitfield<1,1,uint8_t>{};
constexpr auto RV_MEMORY_X = rv_bitfield<1,2,uint8_t>{};

constexpr auto RV_MEMORY_RW = RV_MEMORY_R | RV_MEMORY_W;
constexpr auto RV_MEMORY_RX = RV_MEMORY_R | RV_MEMORY_X;
constexpr auto RV_MEMORY_RWX = RV_MEMORY_R | RV_MEMORY_W | RV_MEMORY_X;

class rv_memory
{
public:
    rv_memory() = delete;
    rv_memory(rv_uint ram_size);
    ~rv_memory();

    void set_region(rv_uint address, const uint8_t* data, size_t len);
    void protect_region(rv_uint address, size_t len, uint8_t prot);

    rv_uint fault_address() const { return fault_address_; }
    rv_exception last_exception() const { return last_exception_; }


    template<typename T> bool fetch(rv_uint address, T& value) const
    {
        if (address <= (ram_end_ - sizeof(T)) && ((mpu_[address >> 12] & RV_MEMORY_RX) == RV_MEMORY_RX))  {
            value = *(T *)(ram_ + address);
            return true;
        }
        fault_address_ = address;
        last_exception_ = rv_exception::instruction_access_fault;
        return false;
    }

    template<typename T> bool read(rv_uint address, T& value) const
    {
        if (address <= (ram_end_ - sizeof(T)) && ((mpu_[address >> 12] & RV_MEMORY_R) == RV_MEMORY_R)) {
            value = *(T *)(ram_ + address);
            return true;
        }
        fault_address_ = address;
        last_exception_ = rv_exception::load_access_fault;
        return false;
    }

    template<typename T> bool write(rv_uint address, T value)
    {
        if (address <= (ram_end_ - sizeof(T)) && ((mpu_[address >> 12] & RV_MEMORY_W) == RV_MEMORY_W)) {
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
    std::vector<uint8_t> mpu_;

    mutable rv_uint fault_address_ = 0;
    mutable rv_exception last_exception_;
};
