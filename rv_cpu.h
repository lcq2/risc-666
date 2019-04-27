#pragma once

#include <limits>
#include <array>
#include "rv_global.h"
#include "rv_memory.h"
#include "rv_sdl.h"

class rv_cpu
{
public:
    rv_cpu() = delete;
    explicit rv_cpu(rv_memory& memory);

    void reset(rv_uint pc = 0);
    void run(size_t nCycles);

    uint64_t cycle_count() const { return cycle_; }

    bool emulation_exit() const { return emulation_exit_; }
    int emulation_exit_status() const { return emulation_exit_status_; }

private:
    uint32_t decode_rd(uint32_t insn) const { return (insn >> 7) & 0x1F; }
    uint32_t decode_rs1(uint32_t insn) const { return (insn >> 15) & 0x1F; }
    uint32_t decode_rs2(uint32_t insn) const { return (insn >> 20) & 0x1F; }
    uint32_t decode_funct3(uint32_t insn) const { return (insn >> 12) & 0b111; }

    // extract [low, high] bits from a 32bit value
    uint32_t bits(uint32_t val, uint32_t low, uint32_t high) const { return (val >> low) & ((1 << (high - low + 1)) - 1); }

    // extract a single bit from a 32bit value
    uint32_t bit(uint32_t val, uint32_t bit) const { return (val >> bit) & 1; }

    void next_insn(rv_uint cnt = 4) { pc_ += cnt; }
    void jump_insn(rv_uint newpc) { pc_ = newpc; }

    void raise_exception(rv_exception code);

    void raise_illegal_instruction() { raise_exception(rv_exception::illegal_instruction); }
    void raise_memory_exception() { raise_exception(memory_.last_exception()); }
    void raise_breakpoint_exception() { raise_exception(rv_exception::breakpoint); }

    inline void execute_lui(uint32_t insn);
    inline void execute_auipc(uint32_t insn);
    inline void execute_jal(uint32_t insn);
    inline void execute_jalr(uint32_t insn);
    inline void execute_branch(uint32_t insn);
    inline void execute_load(uint32_t insn);
    inline void execute_store(uint32_t insn);
    inline void execute_amo(uint32_t insn);
    inline void execute_imm(uint32_t insn);
    inline void execute_op(uint32_t insn);
    inline void execute_misc_mem(uint32_t insn);
    inline void execute_system(uint32_t insn);

    bool csr_read(uint32_t csr, rv_uint& csr_value, bool write_back = false);
    bool csr_write(uint32_t csr, rv_uint csr_value);

    bool csr_rw(uint32_t csr, uint32_t rd, rv_uint new_value, uint32_t csrop);

    void dump_regs();

    void stop_emulation(int exit_code);
    void handle_user_exception();
    void handle_illegal_instruction();
    void handle_memory_access_fault();
    void handle_breakpoint_exception();
    void dispatch_syscall(rv_uint syscall_no,
        rv_uint arg0,
        rv_uint arg1,
        rv_uint arg2,
        rv_uint arg3,
        rv_uint arg4,
        rv_uint arg5
        );

    // syscalls emulation
    rv_uint syscall_fstat(rv_uint arg0, rv_uint arg1);
    rv_uint syscall_stat(rv_uint arg0, rv_uint arg1);
    rv_uint syscall_brk(rv_uint arg0);
    rv_uint syscall_open(rv_uint arg0, rv_uint arg1, rv_uint arg2);
    rv_uint syscall_write(rv_uint arg0, rv_uint arg1, rv_uint arg2);
    rv_uint syscall_read(rv_uint arg0, rv_uint arg1, rv_uint arg2);
    rv_uint syscall_close(rv_uint arg0);
    rv_uint syscall_lseek(rv_uint arg0, rv_uint arg1, rv_uint arg2);
    rv_uint syscall_exit(rv_uint arg0);
    rv_uint syscall_openat(rv_uint arg0, rv_uint arg1, rv_uint arg2, rv_uint arg3);
    rv_uint syscall_gettimeofday(rv_uint arg0, rv_uint arg1);

private:
    rv_uint pc_;
    std::array<rv_uint, 32> regs_;
    rv_uint amo_res_;
    rv_memory& memory_;
    rv_sdl sdl_;

    bool exception_raised_;
    rv_exception exception_code_;

    bool emulation_exit_;
    int emulation_exit_status_;

    // Counter/Timers
    uint64_t time_;
    uint64_t cycle_;
    uint64_t instret_;
};