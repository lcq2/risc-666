#include <memory.h>
#include <limits>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

#include "rv_cpu.h"
#include "rv_exceptions.h"
#include "rv_memory.h"
#include "rv_bits.h"
#include "newlib_syscalls.h"
#include "newlib_trans.h"

constexpr uint32_t kRiscvOpcodeMask = 0x7F;

#define RV_MSTATUS_UIE_SHIFT 0
#define RV_MSTATUS_SIE_SHIFT 1
#define RV_MSTATUS_MIE_SHIFT 3
#define RV_MSTATUS_UPIE_SHIFT 4
#define RV_MSTATUS_SPIE_SHIFT 5
#define RV_MSTATUS_MPIE_SHIFT 7
#define RV_MSTATUS_SPP_SHIFT 8
#define RV_MSTATUS_MPP_SHIFT 11

#define RV_MSTATUS_UIE  (1 << RV_MSTATUS_UIE_SHIFT)
#define RV_MSTATUS_SIE  (1 << RV_MSTATUS_SIE_SHIFT)
#define RV_MSTATUS_MIE  (1 << RV_MSTATUS_MIE_SHIFT)
#define RV_MSTATUS_UPIE (1 << RV_MSTATUS_UPIE_SHIFT)
#define RV_MSTATUS_SPIE (1 << RV_MSTATUS_SPIE_SHIFT)
#define RV_MSTATUS_MPIE (1 << RV_MSTATUS_MPIE_SHIFT)
#define RV_MSTATUS_SPP  (1 << RV_MSTATUS_SPP_SHIFT)
#define RV_MSTATUS_MPP  (3 << RV_MSTATUS_MPP_SHIFT)

constexpr auto RV_MIP_USIP = rv_bitfield<1,0>{};
constexpr auto RV_MIP_SSIP = rv_bitfield<1,1>{};
constexpr auto RV_MIP_MSIP = rv_bitfield<1,3>{};
constexpr auto RV_MIP_UTIP = rv_bitfield<1,4>{};
constexpr auto RV_MIP_STIP = rv_bitfield<1,5>{};
constexpr auto RV_MIP_MTIP = rv_bitfield<1,7>{};
constexpr auto RV_MIP_UEIP = rv_bitfield<1,8>{};
constexpr auto RV_MIP_SEIP = rv_bitfield<1,9>{};
constexpr auto RV_MIP_MEIP = rv_bitfield<1,11>{};

constexpr auto RV_MIE_USIE = rv_bitfield<1,0>{};
constexpr auto RV_MIE_SSIE = rv_bitfield<1,1>{};
constexpr auto RV_MIE_MSIE = rv_bitfield<1,3>{};
constexpr auto RV_MIE_UTIE = rv_bitfield<1,4>{};
constexpr auto RV_MIE_STIE = rv_bitfield<1,5>{};
constexpr auto RV_MIE_MTIE = rv_bitfield<1,7>{};
constexpr auto RV_MIE_UEIE = rv_bitfield<1,8>{};
constexpr auto RV_MIE_SEIE = rv_bitfield<1,9>{};
constexpr auto RV_MIE_MEIE = rv_bitfield<1,11>{};

constexpr auto RV_MCOUNTEREN_CY = rv_bitfield<1,0>{};
constexpr auto RV_MCOUNTEREN_TM = rv_bitfield<1,1>{};
constexpr auto RV_MCOUNTEREN_IR = rv_bitfield<1,2>{};

constexpr uint32_t RV_PRIV_U = 0;
constexpr uint32_t RV_PRIV_S = 1;
constexpr uint32_t RV_PRIV_M = 3;

enum class rv_opcode: uint32_t
{
    lui = 0b01101,
    auipc = 0b00101,
    jal = 0b11011,
    jalr = 0b11001,
    branch = 0b11000,
    load = 0b00000,
    store = 0b01000,
    imm = 0b00100,
    op = 0b01100,
    misc_mem = 0b00011,
    system  = 0b11100,
    amo = 0b01011
};

enum class rv_csr: uint32_t
{
    cycle = 0xC00,
    time = 0xC01,
    instret = 0xC02,
    cycleh = 0xC80,
    timeh = 0xC81,
    instreth = 0xC82,

    mvendorid = 0xF11,
    marchid = 0xF12,
    mimpid = 0xF13,
    mhartid = 0xF14,

    mstatus = 0x300,
    misa = 0x301,
    mie = 0x304,
    mtvec = 0x305,
    mcounteren = 0x306,

    mscratch = 0x340,
    mepc = 0x341,
    mcause = 0x342,
    mtval = 0x343,
    mip = 0x344
};

inline const char *const g_regNames[] = {
    "zero",
    "ra",
    "sp",
    "gp",
    "tp",
    "t0",
    "t1", "t2",
    "s0",
    "s1",
    "a0", "a1",
    "a2", "a3", "a4", "a5", "a6", "a7",
    "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    "t3", "t4", "t5", "t6"
};

enum riscv_register
{
    zero = 0,
    ra,
    sp,
    gp,
    tp,
    t0,
    t1, t2,
    s0,
    s1,
    a0, a1,
    a2, a3, a4, a5, a6, a7,
    s2, s3, s4, s5, s6, s7, s8, s9, s10, s11,
    t3, t4, t5, t6
};

rv_cpu::rv_cpu(rv_memory& memory)
    : memory_{memory}, sdl_{memory}
{

}

void rv_cpu::reset(rv_uint pc)
{
    // execution starts at 0x1000 in user mode
    pc_ = pc;

    // initialize all registers to 0
    regs_.fill(0);

    cycle_ = 0;
    instret_ = 0;

    exception_raised_ = false;

    // setup stack pointer
    // upon entry, on the stack we have: argc, argv
    // newlib does not use envp
    regs_[sp] = memory_.stack_pointer();

    emulation_exit_status_ = 0;
    emulation_exit_ = false;
}

void rv_cpu::run(size_t nCycles)
{
    auto c = nCycles;

    c += 1;
    while(likely(!exception_raised_)) {
        if (unlikely(!--c))
            break;

        uint32_t insn;
        if (unlikely(!memory_.read(pc_, insn))) {
            raise_memory_exception();
            break;
        }

        // add support for compressed instructions!
        const auto opcode = (rv_opcode) ((insn & kRiscvOpcodeMask) >> 2);
        switch (opcode) {
        case rv_opcode::load:
            execute_load(insn);
            break;
        case rv_opcode::misc_mem:
            execute_misc_mem(insn);
            break;
        case rv_opcode::imm:
            execute_imm(insn);
            break;
        case rv_opcode::auipc:
            execute_auipc(insn);
            break;
        case rv_opcode::store:
            execute_store(insn);
            break;
        case rv_opcode::amo:
            execute_amo(insn);
            break;
        case rv_opcode::op:
            execute_op(insn);
            break;
        case rv_opcode::lui:
            execute_lui(insn);
            break;
        case rv_opcode::branch:
            execute_branch(insn);
            break;
        case rv_opcode::jalr:
            execute_jalr(insn);
            break;
        case rv_opcode::jal:
            execute_jal(insn);
            break;
        case rv_opcode::system:
            execute_system(insn);
            break;
        default:
            raise_illegal_instruction();
            break;
        }
    }
    if (unlikely(exception_raised_)) {
        handle_user_exception();
    }
    cycle_ += nCycles - c;
}

void rv_cpu::execute_lui(uint32_t insn)
{
    const auto rd = decode_rd(insn);
    regs_[rd] = insn & 0xFFFFF000;
    next_insn();
}

void rv_cpu::execute_auipc(uint32_t insn)
{
    const auto rd = decode_rd(insn);
    regs_[rd] = pc_ + (int32_t)(insn & 0xFFFFF000);
    next_insn();
}

void rv_cpu::execute_jal(uint32_t insn)
{
    const auto rd = decode_rd(insn);
    rv_int imm = (bits(insn, 21, 30) << 1) |
                 (bit(insn, 20) << 11) |
                 (bits(insn, 12, 19) << 12) |
                 (bit(insn, 31) << 20);
    imm = (imm << 11) >> 11;
    if (rd != 0) {
        regs_[rd] = pc_ + 4;
    }
    jump_insn(pc_ + imm);
}

void rv_cpu::execute_jalr(uint32_t insn)
{
    const rv_int imm = (rv_int)insn >> 20;
    const auto rd = decode_rd(insn);
    const auto rs1 = decode_rs1(insn);

    if (rd != 0) {
        regs_[rd] = pc_ + 4;
    }
    jump_insn((regs_[rs1] + imm) & 0xFFFFFFFE);
}

void rv_cpu::execute_branch(uint32_t insn)
{
    const auto rs1 = decode_rs1(insn);
    const auto rs2 = decode_rs2(insn);
    const auto funct3 = decode_funct3(insn);

    const rv_uint val1 = regs_[rs1];
    const rv_uint val2 = regs_[rs2];
    int cond = 0;
    switch (funct3 >> 1) {
    case 0b00:  // beq | bne
        cond = val1 == val2;
        break;
    case 0b10:  // blt | bge
        cond = (rv_int)val1 < (rv_int)val2;
        break;
    case 0b11:  // bltu | bgeu
        cond = val1 < val2;
        break;
    }
    cond ^= (funct3 & 1);
    if (cond != 0) {
        rv_int imm = (bits(insn, 8, 11) << 1) |
                     (bits(insn, 25, 30) << 5) |
                     (bit(insn, 7) << 11) |
                     (bit(insn, 31) << 12);
        imm = (imm << 19) >> 19;
        jump_insn((pc_ + imm) & 0xFFFFFFFE);
    }
    else {
        next_insn();
    }
}

void rv_cpu::execute_load(uint32_t insn)
{
    const auto rd = decode_rd(insn);
    const auto rs1 = decode_rs1(insn);
    const auto funct3 = decode_funct3(insn);

    const rv_int imm = (rv_int)insn >> 20;
    const rv_uint addr = regs_[rs1] + imm;
    rv_uint val;
    switch (funct3) {
    case 0b000:  // lb
        int8_t i8;
        if (!memory_.read(addr, i8)) {
            raise_memory_exception();
            return;
        }
        val = i8;
        break;
    case 0b001:  // lh
        int16_t i16;
        if (!memory_.read(addr, i16)) {
            raise_memory_exception();
            return;
        }
        val = i16;
        break;
    case 0b010:  // lw
        int32_t i32;
        if (!memory_.read(addr, i32)) {
            raise_memory_exception();
            return;
        }
        val = i32;
        break;
    case 0b100:  // lbu
        uint8_t u8;
        if (!memory_.read(addr, u8)) {
            raise_memory_exception();
            return;
        }
        val = u8;
        break;
    case 0b101:  //lhu
        uint16_t u16;
        if (!memory_.read(addr, u16)) {
            raise_memory_exception();
            return;
        }
        val = u16;
        break;
    default:
        raise_illegal_instruction();
        return;
    }
    if (likely(rd != 0))
        regs_[rd] = val;
    next_insn();
}

void rv_cpu::execute_store(uint32_t insn)
{
    const auto rd = decode_rd(insn);
    const auto rs1 = decode_rs1(insn);
    const auto rs2 = decode_rs2(insn);
    const auto funct3 = decode_funct3(insn);

    const rv_int imm = (rv_int)((insn & 0xFE000000) | (rd << 20)) >> 20;
    const rv_uint addr = regs_[rs1] + imm;
    const rv_uint val = regs_[rs2];
    switch (funct3) {
    case 0b000:  // sb
        if (!memory_.write(addr, (uint8_t)(val & 0xFF))) {
            raise_memory_exception();
        }
        break;
    case 0b001:  // sh
        if (!memory_.write(addr, (uint16_t)(val & 0xFFFF))) {
            raise_memory_exception();
            return;
        }
        break;
    case 0b010:  // sw
        if (!memory_.write(addr, val)) {
            raise_memory_exception();
            return;
        }
        break;
    default:
        raise_illegal_instruction();
        return;
    }
    next_insn();
}

void rv_cpu::execute_imm(uint32_t insn)
{
    const auto rd = decode_rd(insn);
    const auto rs1 = decode_rs1(insn);
    const auto funct3 = decode_funct3(insn);

    const rv_int imm = (rv_int)insn >> 20;
    rv_uint res = 0;
    const rv_uint val = regs_[rs1];
    switch (funct3) {
    case 0b000:  // addi
        res = val + imm;
        break;
    case 0b001:  // slli
/*            if ((imm & 0x5F) != 0) {
                raise_illegal_instruction();
                return;
            }*/
        res  = val << (imm & 0x1F);
        break;
    case 0b010:  // slti
        res = (rv_int)val < imm ? 1 : 0;
        break;
    case 0b011:  // sltiu
        res = val < imm ? 1 : 0;
        break;
    case 0b100:
        res = val ^ imm;
        break;
    case 0b101:  // srai | srli
    {
        if ((imm & 0xFFFFFBE0) != 0) {
            raise_illegal_instruction();
            return;
        }
        if ((imm & 0x400) != 0)
            res = (rv_int)val >> (imm & 0x1F);
        else
            res = val >> (imm & 0x1F);
        break;
    }
    case 0b110:  // ori
        res = val | imm;
        break;
    case 0b111:  // andi
        res = val & imm;
        break;
    }
    if (likely(rd != 0))
        regs_[rd] = res;
    next_insn();
}

void rv_cpu::execute_op(uint32_t insn)
{
    const auto rd = decode_rd(insn);
    const auto rs1 = decode_rs1(insn);
    const auto rs2 = decode_rs2(insn);
    const auto funct3 = decode_funct3(insn);

    const rv_uint imm = insn >> 25;
    const rv_uint val1 = regs_[rs1];
    const rv_uint val2 = regs_[rs2];
    rv_uint res = 0;
    if (imm == 1) {
        const auto sval1 = (rv_int)val1;
        const auto sval2 = (rv_int)val2;
        switch (funct3) {
        case 0b000:  // mul
            res = (rv_uint)((rv_long)sval1 * (rv_long)sval2);
            break;
        case 0b001:  // mulh
            res = (rv_uint)(((rv_long)sval1 * (rv_long)sval2) >> 32);
            break;
        case 0b010:  // mulhsu
            res = (rv_uint)(((rv_long)sval1 * (rv_long)val2) >> 32);
            break;
        case 0b011:  // mulhu
            res = (rv_uint)(((rv_long)val1 * (rv_long)val2) >> 32);
            break;
        case 0b100:  // div
        {
            if (val2 == 0)
                res = (rv_uint)-1;
            else if (val1 == 0x80000000 && val2 == (rv_int)-1)
                res = val1;
            else
                res = sval1 / sval2;
        }
            break;

        case 0b101:  // divu
            if (val2 == 0)
                res = (rv_uint)-1;
            else
                res = val1 / val2;
            break;
        case 0b110:  // rem
        {
            if (val2 == 0)
                res = val1;
            else if (val1 == 0x80000000 && val2 == (rv_int)-1)
                res = 0;
            else
                res = (rv_uint)(sval1 % sval2);
        }
            break;
        case 0b111:  // remu
            if (val2 == 0)
                res = val1;
            else
                res = val1 % val2;
            break;
        }
    }
    else {
        switch (funct3) {
        case 0b000:  // add | sub
        {
            if (unlikely((imm & 0x5F) != 0)) {
                raise_illegal_instruction();
                return;
            }
            if ((imm & 0x20) != 0)  // sub
                res = val1 - val2;
            else  // add
                res = val1 + val2;
        }
            break;
        case 0b001:  // sll
            res = val1 << val2;
            break;
        case 0b010:  // slt
            res = (rv_int)val1 < (rv_int)val2 ? 1 : 0;
            break;
        case 0b011:  // sltu
            res = val1 < val2 ? 1 : 0;
            break;
        case 0b100:  // xor
            res = val1 ^ val2;
            break;
        case 0b101:  // srl | sra
        {
            if ((imm & 0x5F) != 0) {
                raise_illegal_instruction();
                return;
            }
            if ((imm & 0x20) != 0)  // sra
                res = (rv_uint)((rv_int)val1 >> val2);
            else  // srl
                res = val1 >> val2;
        }
            break;
        case 0b110:  // or
            res = val1 | val2;
            break;
        case 0b111:  // and
            res = val1 & val2;
            break;
        }
    }
    if (likely(rd != 0)) {
        regs_[rd] = res;
    }
    next_insn();
}

void rv_cpu::execute_amo(uint32_t insn)
{
    const auto rd = decode_rd(insn);
    const auto rs1 = decode_rs1(insn);
    const auto rs2 = decode_rs2(insn);
    const auto funct5 = insn >> 27;

    const auto addr = regs_[rs1];
    rv_uint val;
    switch (funct5) {
    case 0b00010:  // lr.w
        if (rs2 != 0) {
            raise_illegal_instruction();
            return;
        }
        int32_t i32;
        if (!memory_.read(addr, i32)) {
            raise_memory_exception();
            return;
        }
        val = i32;
        amo_res_ = addr;
        break;

    case 0b00011:  // sc.w
        if (amo_res_ == addr) {
            if (!memory_.write(addr, regs_[rs1])) {
                raise_memory_exception();
                return;
            }
            val = 0;
        }
        else {
            val = 1;
        }
        break;
    case 0b00001:  // amoswap.w
    case 0b00000:  // amoadd.w
    case 0b00100:  // amoxor.w
    case 0b01100:  // amoand.w
    case 0b01000:  // amoor.w
    case 0b10000:  // amomin.w
    case 0b10100:  // amomax.w
    case 0b11000:  // amominu.w
    case 0b11100:  // amomaxu.w
    {
        if (memory_.read(addr, (int32_t&)val)) {
            raise_memory_exception();
            return;
        }
        auto val2 = regs_[rs2];
        switch (funct5) {
        case 0b00001:  // amoswap.w
            break;
        case 0b00000:  // amoadd.w
            val2 = (int32_t)(val + val2);
            break;
        case 0b00100:  // amoxor.w
            val2 = (int32_t)(val ^ val2);
            break;
        case 0b01100:  // amoand.w
            val2 = (int32_t)(val & val2);
            break;
        case 0b01000:  // amoor.w
            val2 = (int32_t)(val | val2);
            break;
        case 0b10000:  // amomin.w
            if ((int32_t)val < (int32_t)val2)
                val2 = (int32_t)val;
            break;
        case 0b10100:  // amomax.w
            if ((int32_t)val > (int32_t)val2)
                val2 = (int32_t)val;
            break;
        case 0b11000:  // amominu.w
            if ((uint32_t)val < (uint32_t)val2)
                val2 = (int32_t)val;
            break;
        case 0b11100:  // amomaxu.w
            if ((uint32_t)val > (uint32_t)val2)
                val2 = (int32_t)val;
            break;

        }
        if (!memory_.write(addr, val2)) {
            raise_memory_exception();
            return;
        }
        if (rd != 0) {
            regs_[rd] = val;
        }
    }
        break;
    default:
        raise_illegal_instruction();
        return;
    }
    next_insn();
}
void rv_cpu::execute_misc_mem(uint32_t insn)
{
    // fence and fence.i are nops
    next_insn();
}

void rv_cpu::execute_system(uint32_t insn)
{
    const auto rd = decode_rd(insn);
    const auto rs1 = decode_rs1(insn);
    const auto funct3 = decode_funct3(insn);
    const uint32_t imm = insn >> 20;

    switch (funct3) {
    case 0:  // ecall | ebreak | mret
        switch (imm) {
        case 0:  // ecall
            if ((insn & 0x000FFF80) != 0)
                raise_illegal_instruction();
            raise_exception(static_cast<rv_exception>(
                                static_cast<uint32_t>(rv_exception::ecall_from_umode) +
                                static_cast<uint32_t>(RV_PRIV_U))
            );
            return;
        case 1:  // ebreak
            if ((insn & 0x000FFF80) != 0)
                raise_illegal_instruction();
            raise_exception(rv_exception::breakpoint);
            return;
        case 0x302:  // mret
            raise_illegal_instruction();
            return;
        default:
            raise_illegal_instruction();
            return;
        }
        break;

    case 1:  // csrrw
    case 2:  // csrrs
    case 3:  // csrrc
        if (!csr_rw(imm, rd, regs_[rs1], funct3))
            return;
        break;
    case 5:  // csrrwi
    case 6:  // csrrsi
    case 7:  // csrrci
        if (!csr_rw(imm, rd, (rv_uint)(rs1 & 0b11111), funct3 - 4))
            return;
        break;
    default:
        raise_illegal_instruction();
        return;
    }

    next_insn();
}

bool rv_cpu::csr_read(uint32_t csr, rv_uint &csr_value, bool write_back)
{
    // these are read-only CSRs
    if ((csr & 0xC00) == 0xC00 && write_back) {
        raise_illegal_instruction();
        return false;
    }

    switch ((rv_csr)csr) {
/*    case rv_csr::cycle:
    case rv_csr::instret:
        if (priv_ < RV_PRIV_M) {
            if ((mcounteren_ & RV_MCOUNTEREN_CY) == 0) {
                raise_illegal_instruction();
                return false;
            }
        }
        csr_value = (rv_uint)(cycle_ & 0xFFFFFFFF);
        break;
    case rv_csr::cycleh:
    case rv_csr::instreth:
        if (priv_ < RV_PRIV_M) {
            if ((mcounteren_ & RV_MCOUNTEREN_IR) == 0) {
                raise_illegal_instruction();
                return false;
            }
        }
        csr_value = (rv_uint)(cycle_ >> 32);
        break;
    case rv_csr::mstatus:
        csr_value = mstatus_;
        break;
    case rv_csr::misa:
        csr_value = misa_;
        break;*/
    case rv_csr::mvendorid:
    case rv_csr::mimpid:
    case rv_csr::mhartid:
        csr_value = 0;
        break;
    default:
        raise_illegal_instruction();
        return false;
    }
    return true;
}

bool rv_cpu::csr_write(uint32_t csr, rv_uint csr_value)
{
    uint32_t mask;
    switch ((rv_csr)csr) {
    default:
        raise_illegal_instruction();
        return false;
    }
    return true;
}

bool rv_cpu::csr_rw(uint32_t csr, uint32_t rd, rv_uint new_value, uint32_t csrop)
{
    rv_uint csrvalue = 0;
    switch (csrop) {
    case 1:
        if (!csr_read(csr, csrvalue, true)) {
            return false;
        }
        if(!csr_write(csr, new_value)) {
            return false;
        }
        break;
    case 2:
        if (!csr_read(csr, csrvalue, new_value != 0)) {
            return false;
        }
        if (new_value != 0) {
            if(!csr_write(csr, csrvalue & ~new_value)) {
                return false;
            }
        }
        break;
    case 3:
        if (!csr_read(csr, csrvalue, new_value != 0)) {
            return false;
        }
        if (new_value != 0) {
            if (!csr_write(csr, csrvalue | new_value)) {
                return false;
            }
        }
        break;
    default:
        return false;
    }
    if (rd != 0) {
        regs_[rd] = csrvalue;
    }
    return true;
}

void rv_cpu::raise_exception(rv_exception code)
{
    exception_code_ = code;
    exception_raised_ = true;
}

void rv_cpu::handle_user_exception()
{
    // handle ecall and dispatch syscall handling
    // simulating machine mode (i.e. the host acts as "machine" mode for
    // the emulated risc-v core)
    switch (exception_code_) {
    case rv_exception::ecall_from_umode:
        dispatch_syscall(regs_[a7], regs_[a0], regs_[a1], regs_[a2], regs_[a3], regs_[a4], regs_[a5]);
        break;
    case rv_exception ::illegal_instruction:
        handle_illegal_instruction();
        break;
    default:
        printf("Diocane!\n");
        break;
    }

    exception_raised_ = false;
    pc_ = pc_+4;
}

void rv_cpu::dump_regs()
{
    fprintf(stderr, "\t");
    for (size_t i = 0; i < 32; i += 8) {
        for (size_t j = 0; j < 8; ++j) {
            fprintf(stderr, "x%d: 0x%08x\t", i+j, regs_[i+j]);
        }
        fprintf(stderr, "\n\t");
    }
    fprintf(stderr, "\n");
    fflush(stderr);
}

void rv_cpu::handle_illegal_instruction()
{
    fprintf(stderr, "[e] error: illegal_instruction at %08x\n", pc_);
    dump_regs();
    emulation_exit_ = true;
    emulation_exit_status_ = 255;
}

// syscall dispatching

// int fstat(int fd, struct stat *statbuf);
rv_uint rv_cpu::syscall_fstat(rv_uint arg0, rv_uint arg1)
{
    struct stat st;
    if (fstat((int)arg0, arg1 != 0 ? &st : nullptr) == -1) {
        return (rv_uint)(-errno);
    }

    if (arg1 != 0) {
        newlib_stat *nst = reinterpret_cast<newlib_stat *>(memory_.ram_ptr(arg1));
        newlib_translate_stat(nst, &st);
    }
    return 0;
}

rv_uint rv_cpu::syscall_stat(rv_uint arg0, rv_uint arg1)
{
    const char *pathname = arg0 != 0 ? reinterpret_cast<const char *>(memory_.ram_ptr(arg0)) : nullptr;
    newlib_stat *nst = reinterpret_cast<newlib_stat*>(memory_.ram_ptr(arg1));
    struct stat st;
    if (stat(pathname, arg1 != 0 ? &st : nullptr) == -1)
        return (rv_uint)(-errno);
    if (arg1 != 0) {
        newlib_translate_stat(nst, &st);
    }
    return 0;
}


// int brk(void *addr);
rv_uint rv_cpu::syscall_brk(rv_uint arg0)
{
    if (arg0 == 0)
        return memory_.brk();
    return memory_.set_brk(arg0) ? memory_.brk() : (rv_uint)(-ENOMEM);
}

rv_uint rv_cpu::syscall_open(rv_uint arg0, rv_uint arg1, rv_uint arg2)
{
    const char *pathname = arg0 != 0 ? reinterpret_cast<const char *>(memory_.ram_ptr(arg0)) : nullptr;
    int flags = (int)arg1;
    int mode = (int)arg2;

    int res = open(pathname, newlib_translate_open_flags(flags), mode);
    if (res == -1)
        return (rv_uint)(-errno);
    return (rv_uint)res;
}

// ssize_t write(int fd, const void *buf, size_t count);
rv_uint rv_cpu::syscall_write(rv_uint arg0, rv_uint arg1, rv_uint arg2)
{
    const void *buf = arg1 != 0 ? memory_.ram_ptr(arg1) : nullptr;
    size_t count = (size_t)arg2;
    int fd = (int)arg0;
    int res = (int)write(fd, buf, count);
    if (res == -1)
        return (rv_uint)(-errno);
    return (rv_uint)res;
}

// ssize_t read(int fd, void *buf, size_t count);
rv_uint rv_cpu::syscall_read(rv_uint arg0, rv_uint arg1, rv_uint arg2)
{
    void *buf = arg1 != 0 ? memory_.ram_ptr(arg1) : nullptr;
    size_t count = (size_t)arg2;
    int fd = (int)arg0;
    int res = (int)read(fd, buf, count);
    if (res == -1)
        return (rv_uint)(-errno);
    return (rv_uint)res;
}

// int close(int fd)
rv_uint rv_cpu::syscall_close(rv_uint arg0)
{
    int fd = (int)arg0;

    // we don't want to close our own stdin, stderrr and stdout :)
    if (fd != 0 && fd != 1 && fd != 2)
        return close(fd) != -1 ? 0 : (rv_uint)(-errno);
    return 0;
}

// void _exit(int _status)
rv_uint rv_cpu::syscall_exit(rv_uint arg0)
{
    emulation_exit_ = true;
    emulation_exit_status_ = (int)arg0;
    return 0;
}

rv_uint rv_cpu::syscall_lseek(rv_uint arg0, rv_uint arg1, rv_uint arg2)
{
    int fd = (int)arg0;
    off_t where = (off_t)arg1;
    int whence = (int)arg2;
    int res = (int)lseek(fd, where, whence);
    if (res == -1)
        return (rv_uint)(-errno);
    return (rv_uint)res;
}

rv_uint rv_cpu::syscall_openat(rv_uint arg0, rv_uint arg1, rv_uint arg2, rv_uint arg3)
{
    int dirfd = (int)arg0;
    const char *pathname = arg1 != 0 ? reinterpret_cast<const char *>(memory_.ram_ptr(arg1)) : nullptr;
    int flags = (int)arg2;
    int mode = (int)arg3;

    int res = openat(dirfd, pathname, flags, mode);
    if (res == -1)
        return (rv_uint)(-errno);
    return (rv_uint)res;
}

rv_uint rv_cpu::syscall_gettimeofday(rv_uint arg0, rv_uint arg1)
{
    struct newlib_timeval *ntv = reinterpret_cast<struct newlib_timeval*>(memory_.ram_ptr(arg0));
    struct timezone *tz = reinterpret_cast<struct timezone*>(memory_.ram_ptr(arg1));
    struct timeval tv;
    int res = gettimeofday(arg0 != 0 ? &tv : nullptr, arg1 != 0 ? tz : nullptr);
    if (res != -1 && arg0 != 0)
        newlib_translate_timeval(ntv, &tv);
    return res != -1 ? 0 : (rv_uint)(-errno);
}

void rv_cpu::dispatch_syscall(rv_uint syscall_no,
    rv_uint arg0,
    rv_uint arg1,
    rv_uint arg2,
    rv_uint arg3,
    rv_uint arg4,
    rv_uint arg5
)
{
#ifdef DEBUG_SYSCALLS
    fprintf(stderr, "[d] syscall %d, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x\n", syscall_no, arg0, arg1,
        arg2, arg3, arg4, arg5);
#endif

    switch (syscall_no) {
    case SYS_fstat:
        regs_[a0] = syscall_fstat(arg0, arg1);
        break;

    case SYS_stat:
        regs_[a0] = syscall_stat(arg0, arg1);
        break;

    case SYS_brk:
        regs_[a0] = syscall_brk(arg0);
        break;

    case SYS_open:
        regs_[a0] = syscall_open(arg0, arg1, arg2);
        break;

    case SYS_read:
        regs_[a0] = syscall_read(arg0, arg1, arg2);
        break;

    case SYS_write:
        regs_[a0] = syscall_write(arg0, arg1, arg2);
        break;

    case SYS_lseek:
        regs_[a0] = syscall_lseek(arg0, arg1, arg2);
        break;

    case SYS_close:
        regs_[a0] = syscall_close(arg0);
        break;

    case SYS_exit:
        regs_[a0] = syscall_exit(arg0);
        break;

    case SYS_openat:
        regs_[a0] = syscall_openat(arg0, arg1, arg2, arg3);
        break;

    case SYS_gettimeofday:
        regs_[a0] = syscall_gettimeofday(arg0, arg1);
        break;

    case SYS_av_init:
        regs_[a0] = sdl_.syscall_init(arg0, arg1);
        break;

    case SYS_av_set_framebuffer:
        regs_[a0] = sdl_.syscall_set_framebuffer(arg0);
        break;

    case SYS_av_delay:
        regs_[a0] = sdl_.syscall_delay(arg0);
        break;

    case SYS_av_update:
        regs_[a0] = sdl_.syscall_update();
        break;

    case SYS_av_set_palette:
        regs_[a0] = sdl_.syscall_set_palette(arg0, arg1);
        break;

    case SYS_av_get_ticks:
        regs_[a0] = sdl_.syscall_get_ticks();
        break;

    case SYS_av_poll_event:
        regs_[a0] = sdl_.syscall_poll_event(arg0);
        break;

    case SYS_av_shutdown:
        regs_[a0] = sdl_.syscall_shutdown();
        break;
    }
}
