#pragma once
#include "rv_global.h"

enum class rv_exception: uint32_t
{
    // interrupts
        machine_software_interrupt = (1U << 31) | 3,
    machine_timer_interrupt = (1U << 31) | 7,
    machine_external_interrupt = (1U << 31) | 11,
    local_interrupt0 = (1U << 31) | 16,
    local_interrupt1 = (1U << 31) | 17,
    local_interrupt2 = (1U << 31) | 18,
    local_interrupt3 = (1U << 31) | 19,
    local_interrupt4 = (1U << 31) | 20,
    local_interrupt5 = (1U << 31) | 21,
    local_interrupt6 = (1U << 31) | 22,
    local_interrupt7 = (1U << 31) | 23,
    local_interrupt8 = (1U << 31) | 24,
    local_interrupt9 = (1U << 31) | 25,
    local_interrupt10 = (1U << 31) | 26,
    local_interrupt11 = (1U << 31) | 27,
    local_interrupt12 = (1U << 31) | 28,
    local_interrupt13 = (1U << 31) | 29,
    local_interrupt14 = (1U << 31) | 30,
    local_interrupt15 = (1U << 31) | 31,

    // exceptions
        instruction_address_misaligned = 0,
    instruction_access_fault = 1,
    illegal_instruction = 2,
    breakpoint = 3,
    load_address_misaligned = 4,
    load_access_fault = 5,
    store_address_misaligned = 6,
    store_access_fault = 7,
    ecall_from_umode = 8,
    ecall_from_smode = 9,
    ecall_from_mmode = 11,
    instruction_page_fault = 12,
    load_page_fault = 13,
    store_page_fault = 15
};